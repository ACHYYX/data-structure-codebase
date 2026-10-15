<#
.SYNOPSIS
    顺序表（SeqList）分支覆盖率一键报告

.DESCRIPTION
    用 g++ 的 --coverage 插桩 + gcov，统计 SeqList.cpp 在 GoogleTest 用例下的
    行覆盖率与分支覆盖率，并列出未被走到的分支。

    为什么不用 MSVC 测：MSVC 没有 gcov 这类命令行插桩工具（VS 的原生 C++ 代码
    覆盖率面板是 Enterprise 功能），而工程里链接的 GoogleTest 是 MSVC 静态库、
    无法配合 gcov。所以脚本用 g++ 把同一份 SeqList.cpp / SeqListGTest.cpp 重新
    编译一遍来测量，被测源码完全一致，结论可直接参考。

    首次运行会把 googletest 源码浅克隆到 %TEMP%\seqlcov\googletest（约十几 MB），
    之后复用。所有中间产物都在临时目录里，不会污染仓库。

.PARAMETER Config
    要测的构建语义：Debug（assert 生效）/ Release（NDEBUG）/ Both（默认）。
    建议以 Release 口径为准 —— Debug 下 assert 宏会凭空多出十几个分支，而且死亡
    测试是在子进程里被 abort 终止的，gcov 数据没机会落盘，两个因素都会让数字失真。

.PARAMETER MinBranch
    分支覆盖率下限（百分比）。低于该值时脚本以退出码 1 结束，便于接入 CI。
    默认 0 表示不做门禁。例如：-MinBranch 90

.EXAMPLE
    .\coverage.ps1
    依次测 Debug 和 Release 两种口径。

.EXAMPLE
    .\coverage.ps1 -Config Release -MinBranch 90
    只测 Release，并且分支覆盖率低于 90% 时返回失败。
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'Both')]
    [string]$Config = 'Both',

    [double]$MinBranch = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ------------------------------------------------------------------ 路径与工具
$solDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$srcDir = Join-Path $solDir 'SeqList_2026_10_14'      # 被测源码目录
$testDir = Join-Path $solDir 'SeqListGTest'           # GoogleTest 工程目录
$slCpp = Join-Path $srcDir 'SeqList.cpp'
$testCpp = Join-Path $testDir 'SeqListGTest.cpp'

foreach ($f in @($slCpp, $testCpp)) {
    if (-not (Test-Path $f)) { throw "找不到源文件：$f" }
}
foreach ($tool in @('g++', 'gcov')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "找不到 $tool，请安装 w64devkit 并把它的 bin 目录加入 PATH"
    }
}

$workRoot = Join-Path $env:TEMP 'seqlcov'
$gtestDir = Join-Path $workRoot 'googletest'
$gtestSrc = Join-Path $gtestDir 'googletest'
$gtestInc = Join-Path $gtestSrc 'include'

# ------------------------------------------------------------------ 辅助函数
# 跑一条命令行（走 cmd，便于使用重定向），失败时抛出
function Invoke-Tool {
    param(
        [Parameter(Mandatory)][string]$CommandLine,
        [switch]$IgnoreError
    )
    $out = & cmd /c "$CommandLine 2>&1"
    if ($LASTEXITCODE -ne 0 -and -not $IgnoreError) {
        Write-Host $out
        throw "执行失败：$CommandLine"
    }
    return $out
}

# 从 gcov -f 的输出里抽出每个 "Function/File '名称'" 后面的统计
function Get-GcovSummary {
    param([string[]]$Lines)
    $result = @()
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        $m = [regex]::Match($Lines[$i], "^Function '(.+)'$")
        if ($m.Success) {
            $kind = 'Function'; $name = $m.Groups[1].Value
        }
        elseif ($Lines[$i] -match "^File '(.+)'$") {
            $kind = 'File'; $name = $Matches[1]
        }
        else { continue }

        $rec = [ordered]@{ 类型 = $kind; 名称 = $name; 行 = ''; 分支 = ''; 分支总数 = 0 }
        for ($j = $i + 1; $j -lt [Math]::Min($i + 6, $Lines.Count); $j++) {
            if ($Lines[$j] -match '^Lines executed:([\d\.]+)% of (\d+)') {
                $rec.行 = [double]$Matches[1]
            }
            elseif ($Lines[$j] -match '^Taken at least once:([\d\.]+)% of (\d+)') {
                $pct = [double]$Matches[1]
                $total = [int]$Matches[2]
                $taken = [int][Math]::Round($total * $pct / 100.0)
                $rec.分支 = $pct
                $rec.分支总数 = $total
                $rec.分支已覆盖 = $taken
            }
        }
        $result += [pscustomobject]$rec
    }
    return $result
}

# 解析 gcov 的注解源码，找出所有"没被走到"的分支（含所在行号与源码）
function Get-UncoveredBranch {
    param([string]$AnnotatedFile)
    $raw = New-Object System.Collections.ArrayList
    $curLine = 0; $curSrc = ''
    foreach ($l in (Get-Content -LiteralPath $AnnotatedFile)) {
        # 形如 "       31*:    8:.assert(ps);"（注意可能有 * 标记）
        if ($l -match '^\s*[-#\d]+\*?:\s*(\d+):(.*)$') {
            $curLine = [int]$Matches[1]; $curSrc = $Matches[2].Trim(); continue
        }
        # 形如 "branch  0 taken 0 (fallthrough)" 或 "branch  3 never executed"
        if ($l -match '^branch\s+\d+\s+(taken\s+0\b|never executed)') {
            [void]$raw.Add([pscustomobject]@{ 行 = $curLine; 源码 = $curSrc })
        }
    }
    return $raw | Group-Object 行 | ForEach-Object {
        [pscustomobject]@{ 行 = [int]$_.Name; 缺失 = $_.Count; 源码 = $_.Group[0].源码 }
    } | Sort-Object 行
}

# ------------------------------------------------------------------ 准备 gtest
Write-Host ''
Write-Host '================ 顺序表分支覆盖率报告 ================' -ForegroundColor Cyan
Write-Host "被测代码: $slCpp"
Write-Host "测试用例: $testCpp"

if (-not (Test-Path (Join-Path $gtestSrc 'src\gtest-all.cc'))) {
    Write-Host '[准备] 首次运行，正在下载 googletest 源码...' -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $workRoot | Out-Null
    Invoke-Tool "git clone --depth 1 --quiet https://github.com/google/googletest.git `"$gtestDir`"" | Out-Null
}

# gtest 自身不带 --coverage，避免它的分支混进统计
$gtestObjs = @()
foreach ($src in @('gtest-all.cc', 'gtest_main.cc')) {
    $obj = Join-Path $workRoot ([IO.Path]::GetFileNameWithoutExtension($src) + '.o')
    $gtestObjs += $obj
    if (-not (Test-Path $obj)) {
        Write-Host "[准备] 编译 $src ..." -ForegroundColor Yellow
        Invoke-Tool "g++ -std=c++20 -O0 -g -I`"$gtestInc`" -I`"$gtestSrc`" -c `"$gtestSrc\src\$src`" -o `"$obj`"" | Out-Null
    }
}

# ------------------------------------------------------------------ 逐个配置测量
$configs = if ($Config -eq 'Both') { @('Debug', 'Release') } else { @($Config) }
$overall = @()

foreach ($cfg in $configs) {
    Write-Host ''
    Write-Host "---------------- $cfg 构建语义 ----------------" -ForegroundColor Green

    $dir = Join-Path $workRoot $cfg
    Remove-Item $dir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $dir | Out-Null

    $defines = if ($cfg -eq 'Release') { '-DNDEBUG' } else { '' }
    $flags = "-std=c++20 -O0 -g --coverage $defines -I`"$gtestInc`" -I`"$testDir`" -I`"$srcDir`""

    Invoke-Tool "g++ $flags -c `"$slCpp`" -o `"$dir\SeqList.o`"" | Out-Null
    Invoke-Tool "g++ $flags -c `"$testCpp`" -o `"$dir\SeqListGTest.o`"" | Out-Null
    Invoke-Tool "g++ --coverage `"$dir\SeqList.o`" `"$dir\SeqListGTest.o`" `"$($gtestObjs[0])`" `"$($gtestObjs[1])`" -o `"$dir\cov.exe`"" | Out-Null

    # 跑测试（gcov 数据在进程正常结束时落盘）
    Push-Location $dir
    try {
        $run = Invoke-Tool 'cov.exe' -IgnoreError
        $run | Select-String '^\[  (PASSED|FAILED)' | ForEach-Object { Write-Host "  $($_.Line.Trim())" }

        Invoke-Tool 'gcov --demangled-names -f -b SeqList.o > gcov.txt' | Out-Null
        $summary = Get-GcovSummary -Lines (Get-Content -LiteralPath (Join-Path $dir 'gcov.txt'))

        $file = $summary | Where-Object { $_.类型 -eq 'File' } | Select-Object -First 1
        $funcs = $summary | Where-Object { $_.类型 -eq 'Function' -and $_.分支总数 -gt 0 }

        Write-Host ''
        Write-Host ('  行覆盖率   : {0:N2}%' -f $file.行)
        Write-Host ('  分支覆盖率 : {0:N2}%  ({1}/{2})' -f $file.分支, $file.分支已覆盖, $file.分支总数) -ForegroundColor Yellow

        Write-Host ''
        Write-Host '  按函数统计：'
        Write-Host ('    {0,-34} {1,10} {2,12}' -f '函数', '行覆盖', '分支覆盖')
        foreach ($fn in ($funcs | Sort-Object 名称)) {
            Write-Host ('    {0,-34} {1,9:N1}% {2,9:N1}%  ({3}/{4})' -f $fn.名称, $fn.行, $fn.分支, $fn.分支已覆盖, $fn.分支总数)
        }

        $annotated = Join-Path $dir 'SeqList.cpp.gcov'
        if (Test-Path $annotated) {
            $gaps = @(Get-UncoveredBranch -AnnotatedFile $annotated)
            Write-Host ''
            Write-Host ("  未被走到的分支：共 {0} 处" -f $gaps.Count) -ForegroundColor Magenta
            foreach ($g in $gaps) {
                Write-Host ('    SeqList.cpp:{0,-4} 缺失 {1} 个分支  |  {2}' -f $g.行, $g.缺失, $g.源码)
            }
        }

        $overall += [pscustomobject]@{
            配置 = $cfg
            行 = $file.行
            分支 = $file.分支
            已覆盖 = $file.分支已覆盖
            总数 = $file.分支总数
        }
    }
    finally {
        Pop-Location
    }
}

# ------------------------------------------------------------------ 汇总与门禁
Write-Host ''
Write-Host '================ 汇总 ================' -ForegroundColor Cyan
Write-Host ('{0,-10} {1,10} {2,12}' -f '配置', '行覆盖率', '分支覆盖率')
foreach ($o in $overall) {
    Write-Host ('{0,-10} {1,9:N2}% {2,11:N2}%' -f $o.配置, $o.行, $o.分支)
}

$failed = $overall | Where-Object { $_.分支 -lt $MinBranch }
if ($MinBranch -gt 0) {
    if ($failed) {
        Write-Host ''
        Write-Host ("分支覆盖率门禁未通过：需要 >= {0}%，实际 {1}" -f $MinBranch, (($failed | ForEach-Object { "$($_.配置) $('{0:N2}' -f $_.分支)%" }) -join ', ')) -ForegroundColor Red
        exit 1
    }
    Write-Host ''
    Write-Host ("分支覆盖率门禁通过（>= {0}%）" -f $MinBranch) -ForegroundColor Green
}
exit 0
