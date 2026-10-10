<#
.SYNOPSIS
    DixiOS Build System - PowerShell Port
#>

param(
    [Parameter(Position = 0)]
    [string]$Action,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ScriptArgs
)

$OUT_DIR = "./out"
$TEMP_BUILD_DIR = "./.build_tmp"
$RELEASE_NAME = "DixiOS-v0.1"

# ---------------------------------------------------------
# 1. إعداد شجرة الملفات المؤقتة بناءً على الإضافات والاستثناءات
# ---------------------------------------------------------
function Prepare-Sources {
    param([string[]]$argsList)

    if (Test-Path $TEMP_BUILD_DIR) {
        Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force
    }
    New-Item -ItemType Directory -Path $TEMP_BUILD_DIR | Out-Null

    $DEFAULT_DIRS = @("Apps", "UI", "UserSpace", "Kernel", "VirtSpace", "Drivers", "ext")

    foreach ($dir in $DEFAULT_DIRS) {
        if (Test-Path "./$dir") {
            Copy-Item -Path "./$dir" -Destination $TEMP_BUILD_DIR -Recurse -Force
        }
    }

    foreach ($arg in $argsList) {
        if ($arg.StartsWith("-")) {
            $REMOVE_PATH = $arg.Substring(1)
            $targetPath = Join-Path $TEMP_BUILD_DIR $REMOVE_PATH
            if (Test-Path $targetPath) {
                Remove-Item -Path $targetPath -Recurse -Force
                Write-Host "[DixiOS Build]: تم استثناء المسار: $REMOVE_PATH" -ForegroundColor Yellow
            }
        }
        elseif ($arg.StartsWith("+")) {
            $ADD_PATH = $arg.Substring(1)
            $fullTargetDir = Join-Path $TEMP_BUILD_DIR $ADD_PATH
            $parentDir = Split-Path $fullTargetDir -Parent
            if (-not (Test-Path $parentDir)) {
                New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
            }
            $sourcePath = ".$ADD_PATH"
            if (Test-Path $sourcePath) {
                Copy-Item -Path $sourcePath -Destination $parentDir -Recurse -Force
                Write-Host "[DixiOS Build]: تم تضمين المسار المخصص: $ADD_PATH" -ForegroundColor Green
            }
        }
    }
}

# ---------------------------------------------------------
# 2. دوال إنشاء التوزيعات (.tar.xz / .img / .iso)
# ---------------------------------------------------------
function Build-TarXz {
    Write-Host "[DixiOS Build]: جاري إنشاء حزمة tar.xz..." -ForegroundColor Cyan
    if (-not (Test-Path $OUT_DIR)) { New-Item -ItemType Directory -Path $OUT_DIR | Out-Null }
    
    $tarPath = Join-Path $OUT_DIR "$RELEASE_NAME.tar.xz"
    # استخدام ضغط سحب البيانات المتاح في النظام أو ضغط الأرشيف
    Compress-Archive -Path "$TEMP_BUILD_DIR/*" -DestinationPath "$OUT_DIR/$RELEASE_NAME.zip" -Force
    # ملاحظة: تم استخدام Compress-Archive كبديل آمن لويندوز (Zip) أو يمكنك استخدام أداة tar المدمجة في ويندوز 10/11 حديثاً:
    tar -acvf $tarPath -C $TEMP_BUILD_DIR . | Out-Null
    
    Write-Host "[DixiOS Build]: تم الحفظ في: $tarPath" -ForegroundColor Green
}

function Build-Img {
    Write-Host "[DixiOS Build]: جاري إنشاء ملف الصورة .img..." -ForegroundColor Cyan
    if (-not (Test-Path $OUT_DIR)) { New-Item -ItemType Directory -Path $OUT_DIR | Out-Null }
    
    $imgPath = Join-Path $OUT_DIR "$RELEASE_NAME.img"
    # محاكاة إنشاء ملف فارغ بحجم 512 ميجابايت في ويندوز
    $stream = [System.IO.File]::Create($imgPath)
    $stream.SetLength(512MB)
    $stream.Close()

    # نسخ الملفات المؤقتة داخلياً (يتطلب أدوات تركيب صور وهمية في ويندوز، لذا ننبه المستخدم أو ننسخ المحتوى كصورة خام)
    Write-Host "[DixiOS Build]: تم إنشاء الهيكل الخام لملف الصورة في: $imgPath" -ForegroundColor Green
}

function Build-Iso {
    Write-Host "[DixiOS Build]: جاري إنشاء ملف الـ ISO الإقلاعي..." -ForegroundColor Cyan
    if (-not (Test-Path $OUT_DIR)) { New-Item -ItemType Directory -Path $OUT_DIR | Out-Null }
    
    $isoPath = Join-Path $OUT_DIR "$RELEASE_NAME.iso"
    
    if (Get-Command "xorriso" -ErrorAction SilentlyContinue) {
        xorriso -as mkisofs -R -J -o $isoPath $TEMP_BUILD_DIR
    } else {
        Write-Host "[DixiOS Error]: أداة xorriso غير متوفرة في بيئة PowerShell الحالية." -ForegroundColor Red
        return $false
    }
    
    Write-Host "[DixiOS Build]: تم الحفظ في: $isoPath" -ForegroundColor Green
}

# ---------------------------------------------------------
# 3. أوامر Makefile الأساسية
# ---------------------------------------------------------
function Invoke-Clean {
    Write-Host "[DixiOS Build]: تنظيف مجلد out والملفات المؤقتة..." -ForegroundColor Yellow
    if (Test-Path $OUT_DIR) { Remove-Item -Path "$OUT_DIR/*" -Recurse -Force -ErrorAction SilentlyContinue }
    if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force -ErrorAction SilentlyContinue }
    Write-Host "[DixiOS Build]: اكتمل التنظيف." -ForegroundColor Green
}

function Build-Bridge {
    Write-Host "[DixiOS Build]: إعادة بناء DixiBridge.jar..." -ForegroundColor Cyan
    $bridgePath = "./UserSpace/System/Bridge"
    if (Test-Path $bridgePath) {
        Push-Location $bridgePath
        
        $javaFiles = Get-ChildItem -Filter "*.java"
        if ($javaFiles) {
            javac *.java 2>$null
        }

        "Main-Class: DixiBridge" | Out-File -Encoding ascii manifest.txt
        jar cfm DixiBridge.jar manifest.txt *.class 2>$null
        
        Remove-Item -Path "manifest.txt" -Force -ErrorAction SilentlyContinue
        Pop-Location
        Write-Host "[DixiOS Build]: تم بناء الجسر بنجاح." -ForegroundColor Green
    } else {
        Write-Host "[DixiOS Error]: لم يتم العثور على مجلد الجسر." -ForegroundColor Red
    }
}

function Show-Help {
    Write-Host "DixiOS Build System (PowerShell Edition)" -ForegroundColor Yellow
    Write-Host "الاستخدام:"
    Write-Host "  .\build.ps1 clean                           - تنظيف المخرجات"
    Write-Host "  .\build.ps1 bridge                          - بناء الجسر فقط"
    Write-Host "  .\build.ps1 tar.xz [المسارات]                - تصدير أرشيف مضغوط"
    Write-Host "  .\build.ps1 img [المسارات]                   - تصدير صورة قرص .img"
    Write-Host "  .\build.ps1 iso [المسارات]                   - تصدير ملف ISO"
    Write-Host "  .\build.ps1 all [المسارات] [صيغة]            - تنفيذ تنظيف وبناء وتجميع"
}

# ---------------------------------------------------------
# 4. معالج الأوامر الرئيسي
# ---------------------------------------------------------
switch ($Action) {
    "clean" {
        Invoke-Clean
    }
    "bridge" {
        Build-Bridge
    }
    "tar.xz" {
        Prepare-Sources $ScriptArgs
        Build-TarXz
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force }
    }
    "img" {
        Prepare-Sources $ScriptArgs
        Build-Img
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force }
    }
    "iso" {
        Prepare-Sources $ScriptArgs
        Build-Iso
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force }
    }
    "all" {
        Invoke-Clean
        Build-Bridge
        
        $FORMAT = "tar.xz"
        $ARGS = @()

        foreach ($arg in $ScriptArgs) {
            if ($arg -eq "tar.xz" -or $arg -eq "img" -or $arg -eq "iso") {
                $FORMAT = $arg
            } else {
                $ARGS += $arg
            }
        }
        
        Prepare-Sources $ARGS
        
        switch ($FORMAT) {
            "tar.xz" { Build-TarXz }
            "img"    { Build-Img }
            "iso"    { Build-Iso }
        }
        
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Path $TEMP_BUILD_DIR -Recurse -Force }
        Write-Host "[DixiOS Build]: اكتمل البناء الشامل بنجاح!" -ForegroundColor Green
    }
    default {
        Show-Help
    }
}
