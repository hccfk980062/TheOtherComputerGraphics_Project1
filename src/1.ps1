# 1. 定義 UTF-8 with BOM 格式
$utf8bom = New-Object System.Text.UTF8Encoding($true)

# 2. 取得所有需要轉換的檔案 (包含 src 與根目錄的 .cpp/.h)
$files = Get-ChildItem -Path "." -Recurse -Include *.cpp, *.h, *.c

Write-Host "開始進行編碼轉換 (UTF-8 with BOM)..." -ForegroundColor Yellow

foreach ($file in $files) {
    # 讀取檔案內容
    $content = [System.IO.File]::ReadAllText($file.FullName)
    # 寫回檔案並強制使用帶 BOM 的 UTF-8
    [System.IO.File]::WriteAllText($file.FullName, $content, $utf8bom)
    Write-Host "已處理: $($file.FullName)" -ForegroundColor Green
}

Write-Host "`n轉換完成！" -ForegroundColor Cyan
Write-Host "按任意鍵繼續..." -ForegroundColor White

# 3. 暫停視窗
$Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown") | Out-Null