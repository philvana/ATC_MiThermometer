# Annexe A — Thermomètres (théorie et mode d’emploi)

Micrologiciel fork **philvana/ATC_MiThermometer**, branche **`setpoint-temperature`**.  
Documentation technique (anglais) : [MJWSD05MMC_ROOM_SETPOINT.md](MJWSD05MMC_ROOM_SETPOINT.md).

---

## 1. Thermomètres — micrologiciel

### 1.1 Choix matériel et logiciel

Les capteurs **Xiaomi MJWSD05MMC** (pile CR2450, affichage LCD, **deux boutons** sur les modèles V2.3 ch/en) utilisent la puce **Telink TLSR8250** (Bluetooth Low Energy). Le micrologiciel d’origine ne convient pas à une campagne de mesure à grande échelle (formats fermés, dépendance à l’écosystème fermé).

Le projet s’appuie sur le firmware communautaire [ATC_MiThermometer](https://github.com/pvvx/ATC_MiThermometer) (référence **pvvx**), avec une **fork** dédiée :

- Dépôt : **https://github.com/philvana/ATC_MiThermometer**
- Branche de fonctionnalité : **`setpoint-temperature`**
- Binaire unique pour le parc MJWSD05MMC (EN) : **`BTE_v58.bin`** (version firmware 5.8)
- Flash **une fois par appareil** via [TelinkMiFlasher](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) (navigateur Chrome / Edge / Opera)
- Possibilité de fixer un **nom court** (XXPP) et l’**adresse MAC** pour dépannage et remplacement simple

**Parc mixte :** les anciens modèles **sans bouton** (ex. LYWSD03MMC) restent en firmware d’origine ou ATC standard : température, humidité, **% pile dans `batt`** — sans consigne ni bouton bas.

---

### 1.2 Adaptation au contexte : pas d’heure à l’écran, consigne pièce

En **diffusion seule** (sans connexion Bluetooth régulière), l’heure affichée sur le LCD **dérive** (pas de NTP, pas de gestion été/hiver) et peut inciter les occupants à des manipulations inutiles.

La fork ne propose plus l’ancienne option `no_clock_display`. Le masquage de l’horloge utilise le réglage TelinkMiFlasher déjà existant :

| Réglage flasher | Effet |
|-----------------|--------|
| **12-hour clock** = **décoché** (off) | Pas d’heure sur le LCD ; ligne basse = **consigne pièce** (°C) ; trame BLE spécifique (voir § 1.6) |
| **12-hour clock** = **coché** (on) | Horloge 12 h AM/PM visible ; ligne basse = **date** ; `batt` = **% pile** comme en standard |

Température, humidité, smiley / pile (symbole) et **téléconfiguration Bluetooth** (bouton **haut**) sont conservés.

**Consigne pièce :** valeur de consigne chauffage pour l’installation (pas la sortie relais du thermomètre). Elle est réglable au **bouton bas** (§ 1.5), affichée en bas d’écran et transmise en MQTT via OpenMQTTGateway.

---

### 1.3 Convention de nommage (rappel)

- **XX** = numéro d’appartement (11, 12, 21, …)
- **PP** = pièce : **S1** salon/salle à manger, **CU** cuisine, **C1–C3** chambres

**Exemple :** `12S1` = appartement 12, salon. Le nom BLE raccourci peut apparaître sous forme `12S1` / `51S1` selon l’affichage OMG (dérivé du nom configuré et de la MAC).

La correspondance **appartement ↔ étage du bâtiment ↔ identifiant plan Grafana** est traitée côté serveur (**Telegraf**), car le plan utilise l’étage (`2S1`) et non le numéro d’appartement seul.

---

### 1.4 Configuration obligatoire (TelinkMiFlasher)

Après flash de **`BTE_v58.bin`**, appliquer sur **chaque** MJWSD05MMC du parc :

| Paramètre | Valeur |
|-----------|--------|
| Type d’appareil | MJWSD05MMC ou MJWSD05MMC_EN |
| **Advertising type** | **PVVX** (indispensable pour OMG / Theengs) |
| **12-hour clock** | **Off** (masque l’heure, active consigne + encodage MQTT) |
| Chiffrement / bindkey | **Off** (scan passif OMG) |
| Long Range | **Off** (compatibilité scan 1M) |
| Nom / MAC | Selon convention XXPP (§ 1.3) |

Puis **Send config**. Vérifier sur l’écran : pas d’heure, **consigne** en bas (ex. 18), pas la date.

**Connexion flasher :** appui **court sur le bouton haut** → symbole BLE → `Connect` dans les ~80 s.

---

### 1.5 Bouton du bas — cycle de consigne

| Bouton | Rôle |
|--------|------|
| **Haut** (côté « connect ») | Connexion TelinkMiFlasher / OTA ; maintien long = changement d’écran LCD ; reset usine si maintien ~5 s **avec le bas enfoncé** |
| **Bas** | **Appui court** : cycle consigne **10 → 18 → 18,5 → … → 23 → 10** °C (uniquement si horloge masquée, § 1.2) |

La consigne est **mémorisée** en EEPROM et **rafraîchie** dans la publicité BLE au prochain cycle.

---

### 1.6 OpenMQTTGateway et Home Assistant

- Passerelle : **OpenMQTTGateway** (ex. ESP32-S3 + W5500), module BT activé.
- Profil décodeur : **`LYWSD03MMC/MJWSD05MMC_PVVX`** (intégré Theengs).
- Topic typique : `home/<gateway>/BTtoMQTT/<MAC>` (MAC sans `:`).

**Quand l’horloge est masquée** (fork + 12-hour clock off), le champ MQTT **`batt` ne est pas le % pile** :

| Usage | Champ MQTT | Règle |
|--------|------------|--------|
| Consigne pièce (°C) | `batt` | **`consigne = batt / 2`** (ex. 18 °C → `batt` = 36) |
| État pile | `volt` | Tension en **volts** (ex. 2,95) — **ne pas** utiliser `batt` pour la pile |

**Capteurs anciens ou horloge affichée :** `batt` = 0–100 % pile ; ne pas appliquer `batt/2` comme consigne.

**Réglage gateway recommandé** (alimentation secteur + Ethernet) : réduire l’intervalle entre scans BLE, ex. `interval` = 30000 ms, `scanduration` = 12000 ms (voir doc technique).

Exemple template Home Assistant (consigne) :

```yaml
state: "{{ (states('sensor.appt_12s1_batt') | int(0)) / 2 }}"
unit_of_measurement: "°C"
```

---

### 1.7 Migration OTA et EEPROM

- Passage depuis firmware ≤ **5.8** : migration automatique — **12-hour clock** forcé à **off**, suppression de l’ancien bit `no_clock_display` s’il était présent.
- **`rs1_invert`** sur GPIO PC4 (bouton bas) : corrigé à la mise à jour pour une détection fiable.

---

### 1.8 Dépannage rapide

| Symptôme | Action |
|----------|--------|
| OMG ne voit pas le capteur | PVVX, pas de Long Range ; rapprocher ; appui bouton **haut** ; plusieurs scans |
| `batt` = 93–100, pas 36 pour 18 °C affiché | Vérifier 12-hour clock **off** et bon binaire **BTE_v58** sur **cette** MAC |
| Date encore visible | Décocher **12-hour clock** → Send config |
| Bouton bas inactif | Flash branche `setpoint-temperature` ; appui **court** ; horloge masquée |
| Parc mixte | Anciens sans bouton : temp/hum/`batt`%=pile ; MJWSD05 flashés : consigne selon § 1.6 |

---

*Révision alignée sur la branche `setpoint-temperature` (commits documentation + firmware room setpoint).*
