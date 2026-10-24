<#
.SYNOPSIS
    DixiOS Build System (PowerShell Version)
#>

$OUT_DIR = "./out"
$TEMP_BUILD_DIR = "./.build_tmp"
$RELEASE_NAME = "DixiOS-v0.1"

# ---------------------------------------------------------
# 1. إعداد شجرة الملفات المؤقتة بناءً على الإضافات والاستثناءات
# ---------------------------------------------------------
function Initialize-Sources {
    param([string[]]$ArgsList)
    
    if (Test-Path $TEMP_BUILD_DIR) {
        Remove-Item -Recurse -Force $TEMP_BUILD_DIR
    }
    New-Item -ItemType Directory -Path $TEMP_BUILD_DIR | Out-Null

    $DEFAULT_DIRS = @("UserSpace/bin", "boot", "out")

    foreach ($dir in $DEFAULT_DIRS) {
        if (Test-Path "./$dir") {
            Copy-Item -Recurse -Force "./$dir" "$TEMP_BUILD_DIR/"
        }
    }

    foreach ($arg in $ArgsList) {
        if ($arg.StartsWith("-")) {
            $REMOVE_PATH = $arg.Substring(1)
            $targetPath = Join-Path $TEMP_BUILD_DIR $REMOVE_PATH
            if (Test-Path $targetPath) {
                Remove-Item -Recurse -Force $targetPath
            }
            Write-Host "[DixiOS Build]: تم استثناء المسار: $REMOVE_PATH"
        }
        elseif ($arg.StartsWith("+")) {
            $ADD_PATH = $arg.Substring(1)
            $targetDir = Join-Path $TEMP_BUILD_DIR (Split-Path $ADD_PATH -Parent)
            if (-not (Test-Path $targetDir)) {
                New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
            }
            $sourcePath = ".$ADD_PATH"
            if (Test-Path $sourcePath) {
                Copy-Item -Recurse -Force $sourcePath "$targetDir/"
                Write-Host "[DixiOS Build]: تم تضمين المسار المخصص: $ADD_PATH"
            }
        }
    }
}

# ---------------------------------------------------------
# 2. دالة إنشاء التوزيعة (.tar.xz)
# ---------------------------------------------------------
function Build-TarXz {
    Write-Host "[DixiOS Build]: جاري إنشاء حزمة tar.xz..."
    if (-not (Test-Path $OUT_DIR)) {
        New-Item -ItemType Directory -Path $OUT_DIR | Out-Null
    }
    tar -cJvf "$OUT_DIR/$RELEASE_NAME.tar.xz" -C $TEMP_BUILD_DIR .
    Write-Host "[DixiOS Build]: تم الحفظ في: $OUT_DIR/$RELEASE_NAME.tar.xz"
}

# ---------------------------------------------------------
# 3. أوامر التنظيف والمساعدة
# ---------------------------------------------------------
function Clear-Build {
    Write-Host "[DixiOS Build]: تنظيف مجلد out والملفات المؤقتة..."
    if (Test-Path $OUT_DIR) {
        Get-ChildItem $OUT_DIR | Remove-Item -Recurse -Force
    }
    if (Test-Path $TEMP_BUILD_DIR) {
        Remove-Item -Recurse -Force $TEMP_BUILD_DIR
    }
    Write-Host "[DixiOS Build]: اكتمل التنظيف."
}

function Show-Help {
    Write-Host "DixiOS Build System"
    Write-Host "الاستخدام:"
    Write-Host "  powershell -File build.ps1 clean                           - تنظيف المخرجات"
    Write-Host "  powershell -File build.ps1 tar.xz [المسارات]                - تصدير أرشيف مضغوط"
    Write-Host "  powershell -File build.ps1 all [المسارات]                  - تنفيذ تنظيف وبناء وتجميع"
}

# ---------------------------------------------------------
# 4. معالج الأوامر الرئيسي
# ---------------------------------------------------------
$ACTION = $args[0]
$PARAMS = if ($args.Length -gt 1) { $args[1..($args.Length - 1)] } else { @() }

switch ($ACTION) {
    "clean" {
        Clear-Build
    }
    "tar.xz" {
        Initialize-Sources $PARAMS
        Build-TarXz
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Recurse -Force $TEMP_BUILD_DIR }
    }
    "all" {
        Clear-Build
        Initialize-Sources $PARAMS
        Build-TarXz
        if (Test-Path $TEMP_BUILD_DIR) { Remove-Item -Recurse -Force $TEMP_BUILD_DIR }
        Write-Host "[DixiOS Build]: اكتمل البناء الشامل بنجاح!"
    }
    default {
        Show-Help
    }
}
