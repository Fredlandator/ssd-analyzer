# SSD Analyzer Plugin pour Saleae Logic 2

Plugin d'analyse pour le protocole SSD (Slot Car Digital) compatible avec les analyseurs logiques Saleae Logic 2.

![Version](https://img.shields.io/badge/version-1.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/license-GPL--3.0-green.svg)

## 📋 Description

Ce plugin permet de décoder et d'analyser le protocole de communication utilisé par les voitures de slot digitales (SSD). Il offre un décodage complet des paquets SSD avec visualisation en temps réel des commandes et données des voitures.

### ✨ Fonctionnalités principales

- **🏁 Mode RACE** : Décodage des commandes de course pour 6 voitures
- **⚙️ Mode PROGRAMMATION** : Décodage des commandes de configuration ID
- **🎨 Couleurs intelligentes** : Codes couleur cohérents par type de paquet
- **🔍 Détection d'erreurs** : Vérification du timing, framing et checksum
- **📊 Affichage détaillé** : Décodage complet des données voitures (vitesse, freinage, changement de voie)
- **🛠️ Paramètres ajustables** : Tolerance de timing et calibration PPM
- **📤 Export de données** : Export en CSV pour analyse ultérieure

## 🎯 Protocole SSD

### Structure des paquets
```
Préambule (12-18 bits '1') + Start bit (0) + Commande + [Données voitures] + Checksum
```

### Timing des bits
- **Bit 1** : 57-63μs par demi-bit (période totale : 114-126μs)
- **Bit 0** : 106-125μs par demi-bit (période totale : 212-250μs)

### Format des données voiture
```
Bit 7    : Freinage (0=désactivé, 1=activé)
Bit 6    : Changement de voie (0=non, 1=oui)
Bits 5-0 : Vitesse (si freinage désactivé) ou Puissance freinage (si activé)
```

### Types de commandes
- **0x01 (PROGRAM)** : Programmation ID voiture (4 bytes identiques)
- **0x02 (RACE)** : Course (6 voitures)

## 🚀 Installation

### Prérequis
- **Saleae Logic 2** (dernière version)
- **Windows** : Visual Studio 2019+ avec support C++
- **macOS** : Xcode avec command line tools
- **Linux** : GCC et CMake 3.11+

### Compilation

#### Windows
```batch
# Cloner le projet
git clone https://github.com/votre-username/ssd-analyzer.git
cd ssd-analyzer

# Compiler
build.bat
```

#### macOS/Linux
```bash
# Cloner le projet
git clone https://github.com/votre-username/ssd-analyzer.git
cd ssd-analyzer

# Compiler
mkdir build && cd build
cmake ..
cmake --build .
```

### Installation dans Logic 2

1. **Compilez** le plugin (voir ci-dessus)
2. **Localisez** le fichier `ssd_analyzer.dll` (Windows) ou `ssd_analyzer.so` (macOS/Linux)
3. **Ouvrez Logic 2**
4. **Extensions** → **"Install Custom Analyzer"**
5. **Sélectionnez** votre fichier compilé
6. **Redémarrez** Logic 2

## 🎛️ Utilisation

### Configuration de base

1. **Connectez** votre signal SSD à un canal de l'analyseur
2. **Ajoutez** l'analyseur "SSD" dans Logic 2
3. **Configurez** les paramètres :
   - **Canal d'entrée** : Sélectionnez le canal connecté
   - **Taille préambule** : 14 bits (ajustable 8-22)
   - **Mode timing** : Standard ou Tolérant
   - **Polarité du signal** : Normal (ou Inversé si nécessaire)
   - **Calibration PPM** : 0 (ajustement fin si nécessaire)

### Connexion matérielle

Pour un signal SSD différentiel 0-3.3V :
```
Signal SSD+ ----[R1: 1kΩ]---- Canal analyseur
             |
Signal SSD- ----[R2: 2.2kΩ]---- GND analyseur
```

⚠️ **Attention** : Respectez les spécifications d'entrée de votre analyseur !

### Interprétation des résultats

#### Code couleur
- **🟢 VERT** : Paquets RACE et données associées
- **🔵 BLEU** : Paquets PROGRAM et données associées  
- **🟠 ORANGE** : Commandes inconnues
- **🔴 ROUGE** : Erreurs (timing, framing, checksum)
- **⚪ GRIS** : Éléments neutres (préambule, bits start/end)

#### Affichage des données
- **P** : Préambule
- **S** : Bit de start
- **CMD:RACE/PROGRAM** : Type de commande
- **Car1: S:32 L:---** : Voiture 1, Vitesse 32, pas de changement de voie
- **Car2: B:P2 L:CHG** : Voiture 2, Freinage puissance 2, changement de voie
- **✓ CHK** : Checksum valide
- **✗ CHK** : Erreur de checksum

### Export des données

Le plugin supporte l'export CSV avec les colonnes :
- **Time [s]** : Horodatage
- **Type** : Type de frame (PREAMBLE, COMMAND, CAR_DATA, etc.)
- **Data** : Valeur décimale
- **Hex** : Valeur hexadécimale  
- **Details** : Informations décodées (vitesse, freinage, etc.)

## ⚙️ Configuration avancée

### Paramètres disponibles

| Paramètre | Valeurs | Description |
|-----------|---------|-------------|
| **Canal d'entrée** | Channel 0-7 | Canal connecté au signal SSD |
| **Taille préambule** | 8-22 bits | Longueur minimale du préambule (défaut: 14) |
| **Mode timing** | Standard/Tolérant | Tolérance pour signaux bruités |
| **Polarité du signal** | Normal/Inversé | Gestion automatique des signaux inversés |
| **Calibration PPM** | ±100000 | Correction fine du timing (défaut: 0) |
| **Afficher détails** | Oui/Non | Décodage détaillé des données voitures |

### Ajustement du timing

Si vous rencontrez des erreurs de décodage :

1. **Signaux précis** : Mode "Standard"
2. **Signaux bruités** : Mode "Tolérant" 
3. **Décalage constant** : Ajustez la "Calibration PPM"
4. **Problèmes de synchronisation** : Réduisez la "Taille préambule"

## 🔧 Dépannage

### Problèmes courants

#### Pas de décodage
- ✅ Vérifiez la connexion du signal
- ✅ Ajustez la "Taille préambule" (essayez 12 ou 10 bits)
- ✅ Utilisez le mode "Tolérant"
- ✅ Vérifiez la fréquence d'échantillonnage (min 1 MHz)

#### Erreurs de timing
- ✅ Vérifiez l'intégrité du signal
- ✅ Ajustez la "Calibration PPM"
- ✅ Augmentez la fréquence d'échantillonnage

#### Erreurs de checksum
- ✅ Vérifiez que toutes les voitures sont décodées
- ✅ Ajustez la "Taille préambule" si le nombre détecté diffère de la configuration
- ✅ Vérifiez la polarité du signal

#### Signal inversé
- ✅ Changez le paramètre "Polarité du signal" de "Normal" vers "Inversé" dans les settings du plugin
- ✅ Le plugin gère automatiquement les deux polarités de signal

### Validation

Le plugin inclut un générateur de simulation qui crée automatiquement des paquets SSD de test pour valider le décodage sans signal réel.

## 👨‍💻 Développement

### Architecture du code

```
src/
├── SSDAnalyzer.cpp/.h           # Analyseur principal et décodage
├── SSDAnalyzerSettings.cpp/.h   # Interface de configuration
├── SSDAnalyzerResults.cpp/.h    # Affichage et export des résultats
└── SSDSimulationDataGenerator.cpp/.h # Générateur de données de test
```

### Contribuer

1. **Fork** le projet
2. **Créez** une branche pour votre fonctionnalité
3. **Commitez** vos changements
4. **Poussez** vers la branche
5. **Ouvrez** une Pull Request

### Modification du protocole

Pour adapter à d'autres protocoles similaires :

1. **Timing** : Modifiez les constantes dans `Setup()`
2. **Structure** : Adaptez la machine d'état dans `WorkerThread()`
3. **Données** : Modifiez `DecodeCarData()` pour votre format
4. **Couleurs** : Ajustez `GetCurrentPacketColor()`

## 📄 Spécifications techniques

### Timing supporté
- **Fréquence d'échantillonnage** : 1 MHz minimum (10 MHz recommandé)
- **Bit 1** : 57-63μs par demi-bit (tolérant : 55-65μs)
- **Bit 0** : 106-125μs par demi-bit (tolérant : 104-127μs)
- **Correction PPM** : ±100000 PPM

### Limites
- **Voitures max** : 6 (mode RACE)
- **Données program** : 4 bytes identiques
- **Préambule** : 8-22 bits configurables
- **Checksum** : XOR des données uniquement (commande exclue)

## 📚 Références

- **Spécification SSD** : Voir `reformulation protocole SSD.docx`
- **Saleae Logic 2** : [Documentation officielle](https://support.saleae.com/)
- **Analyzer SDK** : [Guide développeur](https://github.com/saleae/AnalyzerSDK)

## 🐛 Signaler un bug

Ouvrez une issue sur GitHub avec :
- **Version** de Logic 2
- **Configuration** de l'analyseur
- **Capture d'écran** ou fichier de trace
- **Description** du problème

## 📜 Licence

Ce projet est sous licence GPL-3.0. Voir le fichier `LICENSE` pour plus de détails.

## 👥 Auteurs

- **Développeur principal** : [Votre nom]
- **Basé sur** : Analyseur DCC NMRA

## 🙏 Remerciements

- Communauté Saleae pour l'Analyzer SDK
- Projet DCC Analyzer original pour l'inspiration
- Contributeurs et testeurs du protocole SSD

---

**📞 Support** : Pour questions techniques, consultez la documentation ou ouvrez une issue GitHub.