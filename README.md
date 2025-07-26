SSD Analyzer Plugin pour Saleae Logic 2
Plugin d'analyse pour le protocole SSD (Slot Car Digital) compatible avec les analyseurs logiques Saleae Logic 2.

Description
Ce plugin décode le protocole de communication utilisé par les voitures de slot digitales. Il supporte :

Mode RACE : Contrôle de 6 voitures avec données de vitesse, freinage et changement de voie
Mode PROGRAMMATION : Configuration de l'ID des voitures
Détection d'erreurs : Vérification du timing, du framing et du checksum
Affichage détaillé : Décodage complet des données voitures
Protocole SSD
Timing des bits
Bit 1 : ~58μs par demi-bit (total ~116μs)
Bit 0 : ~110μs par demi-bit (total ~220μs)
Structure des paquets
Préambule (14 bits '1') + Start bit (0) + Commande + [Données voitures] + Checksum
Format des données voiture
Bit 7 : Freinage (0=désactivé, 1=activé)
Bit 6 : Changement de voie (0=non, 1=oui)
Bits 5-0 : Vitesse (si freinage désactivé) ou Puissance freinage (si activé)
Installation
Prérequis
Visual Studio 2019 ou plus récent avec support C++
CMake 3.11 ou plus récent
Git
Compilation
Cloner le projet :
bash
git clone [url-du-projet]
cd ssd-analyzer
Compiler sur Windows :
bash
build.bat
Ou manuellement :
bash
mkdir build
cd build
cmake -A x64 ..
cmake --build . --config Release
Localiser le plugin : Le fichier ssd_analyzer.dll sera dans build/Analyzers/Release/
Installation dans Logic 2
Ouvrir Logic 2
Aller dans : Extensions → Install Custom Analyzer
Sélectionner le fichier ssd_analyzer.dll
Redémarrer Logic 2
Utilisation
Configuration
Connecter le signal SSD à un canal de votre analyseur Saleae
Ajouter l'analyseur SSD dans Logic 2
Configurer les paramètres :
Canal d'entrée : Sélectionner le canal connecté au signal SSD
Taille préambule : 14 bits (recommandé)
Mode timing : Standard ou Tolérant
Facteur de calibration : 0 PPM (par défaut)
Afficher détails voitures : Activé pour voir les détails décodés
Connexion Hardware
Pour un signal 0-3.3V différentiel :

Signal SSD+ ----[Résistance 1kΩ]---- Canal analyseur
             |
Signal SSD- ----[Résistance 2.2kΩ]---- GND analyseur
⚠️ Attention : Respectez les spécifications d'entrée de votre analyseur Saleae !

Interprétation des résultats
Affichage des bulles
P : Préambule
S : Bit de start
C : Commande (RACE/PROGRAM)
Car1-6 : Données des voitures avec détails
✓/X : Checksum valide/invalide
E : Fin de paquet
Données voiture décodées
S:32 L:--- : Vitesse 32, pas de changement de voie
B:P2 L:CHG : Freinage puissance 2, changement de voie
S:0 L:--- : Arrêt, pas de changement de voie
Codes d'erreur
B : Erreur de timing de bit
F : Erreur de framing
C : Erreur de checksum
P : Erreur de paquet
Export des données
Le plugin supporte l'export en CSV avec les colonnes :

Time [s] : Horodatage
Type : Type de frame
Data : Valeur décimale
Hex : Valeur hexadécimale
Details : Informations décodées
Dépannage
Problèmes courants
Pas de décodage :

Vérifier la connexion du signal
Ajuster le facteur de calibration PPM
Utiliser le mode "Tolérant" pour les signaux bruités
Erreurs de timing :

Vérifier la fréquence d'échantillonnage (min 1 MHz)
Ajuster le facteur de calibration
Vérifier l'intégrité du signal
Erreurs de checksum :

Signal corrompu ou bruit
Problème de synchronisation
Vérifier les connexions
Validation
Le plugin inclut un générateur de simulation pour tester le décodage sans signal réel.

Développement
Structure du code
src/
├── SSDAnalyzer.cpp/.h           # Analyseur principal
├── SSDAnalyzerSettings.cpp/.h   # Configuration
├── SSDAnalyzerResults.cpp/.h    # Affichage des résultats
└── SSDSimulationDataGenerator.cpp/.h # Générateur de test
Modification du protocole
Pour adapter à d'autres protocoles similaires, modifier :

Les constantes de timing dans SSDAnalyzer.cpp
La structure des paquets dans WorkerThread()
Le décodage des données dans DecodeCarData()
Support
Pour les questions techniques :

Vérifier la documentation Saleae Logic 2
Consulter les spécifications du protocole SSD
Tester avec les données de simulation intégrées
Licence
Ce projet est basé sur l'analyseur DCC original et adapté pour le protocole SSD.

