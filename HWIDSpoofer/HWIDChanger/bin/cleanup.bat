@echo off
echo Cleaning up temporary test files...

del _*.txt 2>nul
del _*.cfg 2>nul
del test*.cfg 2>nul
del inject_test.cfg 2>nul
del final.cfg 2>nul
del deep_test.ps1 2>nul
del test_all.ps1 2>nul
del verify_test.ps1 2>nul
del test_hwid.ps1 2>nul
del strict_test.bat 2>nul
del run_test.bat 2>nul

echo Done!
echo.
echo Remaining files:
dir /b *.exe *.dll *.bat *.txt *.dat 2>nul

