Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Downloading DixiOS (Home Branch)..." -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# التأكد من توفر أداة git في النظام
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Error: git is not installed or not found in PATH!" -ForegroundColor Red
    exit 1
}

# حذف المجلد القديم إذا كان موجوداً مسبقاً
if (Test-Path "DixiOS-Latest") {
    Write-Host "Removing existing 'DixiOS-Latest' folder..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "DixiOS-Latest"
}

# استنساخ الفرع مباشرة إلى المجلد المطلوب
git clone --branch Home --single-branch https://github.com/BrCraft131313/DixiOS.git DixiOS-Latest

Write-Host "=========================================" -ForegroundColor Green
Write-Host " Done! All files are ready in 'DixiOS-Latest'." -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
