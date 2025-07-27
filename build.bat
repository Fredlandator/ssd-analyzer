@echo off
echo ============================================================================
echo  SSD Analyzer Plugin - VERSION FINALE avec checksum corrige
echo ============================================================================
echo.
echo TOUTES LES CORRECTIONS APPLIQUEES:
echo ==================================
echo 1. Ajout de l'etat FSTATE_DSBIT_CHECKSUM pour le bit start avant checksum
echo 2. Correction du calcul checksum: 0xFF ⊕ Commande ⊕ Donnees voitures  
echo 3. Correction de la portee de variable 'lookahead' dans FSTATE_PEBIT
echo 4. Amelioration de la synchronisation entre paquets
echo 5. Simulateur corrige avec les bons checksums
echo.
echo CHECKSUMS CORRIGES:
echo ===================
echo - 6 voitures a 0x80: Checksum = 0xFD (au lieu de 0x00)
echo - 6 voitures a 0x00: Checksum = 0xFD (au lieu de 0x00)
echo - PROGRAM ID=3:      Checksum = 0xFE (au lieu de 0x00)
echo.

REM Verification CMake
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERREUR: CMake n'est pas installe ou pas dans le PATH
    echo Telechargez CMake depuis: https://cmake.org/download/
    pause
    exit /b 1
)

REM Nettoyage optionnel
if "%1"=="clean" (
    echo Nettoyage du dossier build...
    if exist build (
        rmdir /s /q build
    )
    echo Dossier build nettoye.
    echo.
)

REM Creation du dossier build
if not exist build (
    echo Creation du dossier build...
    mkdir build
)

echo Entree dans le dossier build...
cd build

echo.
echo ============================================================================
echo  ETAPE 1: Configuration CMake
echo ============================================================================

cmake -A x64 ..
if %errorlevel% neq 0 (
    echo ERREUR: La configuration CMake a echoue!
    echo.
    echo Solutions possibles:
    echo - Verifiez que Visual Studio 2019+ est installe
    echo - Verifiez que les outils de developpement C++ sont installes
    echo - Verifiez que le SDK Windows est installe
    pause
    exit /b 1
)

echo.
echo ============================================================================
echo  ETAPE 2: Compilation
echo ============================================================================

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERREUR: La compilation a echoue!
    echo.
    echo Solutions possibles:
    echo - Verifiez les erreurs de compilation ci-dessus
    echo - Assurez-vous que tous les fichiers source sont presents
    echo - Verifiez que le SDK Saleae est correctement telecharge
    pause
    exit /b 1
)

echo.
echo ============================================================================
echo  COMPILATION REUSSIE - VERSION FINALE!
echo ============================================================================
echo.

if exist "Analyzers\Release\ssd_analyzer.dll" (
    echo Plugin compile: Analyzers\Release\ssd_analyzer.dll
    
    for %%A in ("Analyzers\Release\ssd_analyzer.dll") do (
        echo Taille du fichier: %%~zA bytes
    )
    
    echo.
    echo ============================================================================
    echo  CORRECTIONS FINALES APPLIQUEES
    echo ============================================================================
    echo.
    echo 1. CALCUL CHECKSUM CORRIGE:
    echo    - Ancienne methode: XOR des donnees voitures uniquement
    echo    - Nouvelle methode: 0xFF ⊕ Commande ⊕ Donnees voitures
    echo    - Plus de faux positifs d'erreurs checksum!
    echo.
    echo 2. MACHINE D'ETAT CORRIGEE:
    echo    - Nouvel etat FSTATE_DSBIT_CHECKSUM
    echo    - Gestion correcte du bit start avant checksum
    echo    - Synchronisation amelioree entre paquets
    echo.
    echo 3. SIMULATEUR CORRIGE:
    echo    - Genere des paquets avec les bons checksums
    echo    - Permet de tester le plugin sans signal reel
    echo.
    echo ============================================================================
    echo  TESTS DE VALIDATION RECOMMANDES
    echo ============================================================================
    echo.
    echo 1. TEST CHECKSUM DE BASE:
    echo    Signal: 6 voitures a 0x80 (freinage)
    echo    Checksum attendu: 0xFD
    echo    Resultat: Doit afficher "Checksum OK"
    echo.
    echo 2. TEST PAQUETS RACE:
    echo    - Testez avec differentes vitesses
    echo    - Verifiez qu'il n'y a plus d'erreurs checksum
    echo    - Testez les changements de voie et freinage
    echo.
    echo 3. TEST PAQUETS PROGRAM:
    echo    - Programmation ID 1 a 6
    echo    - Verification de la double emission
    echo    - Checksums corrects pour chaque ID
    echo.
    echo 4. TEST SIGNAUX CONSECUTIFS:
    echo    - Sequences de paquets multiples
    echo    - Verification de la synchronisation
    echo    - Pas de perte de preambule
    echo.
    
    echo ============================================================================
    echo  INSTALLATION DANS LOGIC 2
    echo ============================================================================
    echo.
    echo 1. Ouvrez Saleae Logic 2
    echo 2. Menu Extensions ^> "Install Custom Analyzer"
    echo 3. Selectionnez: %CD%\Analyzers\Release\ssd_analyzer.dll
    echo 4. Redemarrez Logic 2
    echo 5. L'analyseur "SSD" sera disponible dans la liste
    echo.
    
    echo ============================================================================
    echo  VALIDATION DES CHECKSUMS
    echo ============================================================================
    echo.
    echo Utilisez ces valeurs pour verifier que le plugin fonctionne:
    echo.
    echo RACE - 6 voitures a 0x00 (arret):
    echo   Checksum = 0xFF ⊕ 0x02 ⊕ (6×0x00) = 0xFD
    echo.
    echo RACE - 6 voitures a 0x80 (freinage):
    echo   Checksum = 0xFF ⊕ 0x02 ⊕ (6×0x80) = 0xFD
    echo.
    echo RACE - 6 voitures a 0x3F (vitesse max):
    echo   Checksum = 0xFF ⊕ 0x02 ⊕ (6×0x3F) = 0xC1
    echo.
    echo PROGRAM - ID 3 (4×0x03):
    echo   Checksum = 0xFF ⊕ 0x01 ⊕ (4×0x03) = 0xFE
    echo.
    
    echo ============================================================================
    echo  DEPANNAGE
    echo ============================================================================
    echo.
    echo Si des erreurs persistent:
    echo.
    echo 1. VERIFIEZ LE SIGNAL:
    echo    - Amplitude ^> 1V, fronts nets
    echo    - Frequence echantillonnage ^>= 1 MHz
    echo.
    echo 2. AJUSTEZ LES PARAMETRES:
    echo    - Mode "Tolerant" pour signaux bruiteux
    echo    - Calibration PPM pour drift d'horloge
    echo    - Preambule reduit (10-12 bits) si problemes sync
    echo.
    echo 3. TESTEZ AVEC LA SIMULATION:
    echo    - Le plugin genere automatiquement des donnees test
    echo    - Permet de valider sans signal reel
    echo.
    
) else (
    echo ERREUR: Le fichier ssd_analyzer.dll n'a pas ete trouve!
    echo Verifiez les erreurs de compilation ci-dessus.
)

echo.
echo ============================================================================
echo Tapez ENTREE pour continuer...
echo ============================================================================
pause