# Parabole Acoustique avec Filtre NLMS sur ESP32

Ce projet consiste en la réalisation d'un dispositif d'écoute directionnelle basé sur une parabole acoustique équipée de microphones MEMS et un ESP32, pour explorer des techniques de débruitage en temps réel. Le projet inclut un filtre **NLMS (Normalized Least Mean Squares)** implémenté sur l'ESP32, optimisé pour une exécution efficace grâce aux capacités DSP du microcontrôleur.

## Contenu du dépôt

- **Code source** : Implémentation du filtre NLMS pour le débruitage audio.
- **Fichiers CAO** : Modèles 3D pour l'impression de la structure mécanique (format `.amf`).
- **Documentation** : Instructions pour la construction du dispositif et la mise en œuvre du code.

## Liste des composants nécessaires

1. **ESP32S3** : Une carte de développement avec l'ESP32 S3.
2. **Microphones MEMS** : Deux micros INMP441 ou équivalent avec sortie I2S sur breakout board (Ali).
3. **Décodeur I2S** : un décodeur audio UDA1334A sur breakout board (Ali)
5. **Parabole acoustique** : Imprimée en 3D à partir des fichiers AMF fournis dans le repertoire CAO.
6. **Alimentation** : Une batterie externe USB fait l'affaire.
7. **Câbles Dupont** : Pour connecter les modules et l'ESP32.

## Fonctionnalités principales

- Détection directionnelle du son grâce à une parabole acoustique.
- Débruitage en temps réel avec un filtre NLMS, permettant d'isoler un signal utile.
- Exploitation des capacités DSP de l'ESP32 pour une application efficace du filtre NLMS.

## Structure du dépôt

- `src/` : Code source en C pour l'ESP32.
- CAO/` : Fichiers `.amf` pour l'impression 3D de la parabole et du support.

## Instructions

### 1. Construction mécanique

Imprimez la parabole et les supports à partir des fichiers dans le dossier CAO/`. Les pièces ont été conçues pour s'assembler facilement. Vous trouverez des photos du dispositif ici : https://selvasystems.net/temps-reel-comment-bien-espionner-vos-voisins-parabole-acoustique-et-filtre-nlms
Le mieux est d'utiliser un pied d'appareil photo pour tenir la parabole. Un sabot est prévu pour assurer la liaison avec le pied.

### 2. Déploiement du code

1. Clonez ce dépôt :
   ```bash
   git clone <lien-du-repo>
   ```
2. Configurez l'environnement ESP-IDF.
3. Compilez et téléversez le code sur l'ESP32 :
   ```bash
   idf.py build && idf.py flash
   ```

### 3. Utilisation

Connectez un casque à la sortie audio, et expérimentez le débruitage et l'amplification. Ca marche plutot bien. 
Le but est de fournir une base permettant facilement d'essayer différentes solutions.
## Plus d'informations

Pour plus de détails sur le fonctionnement du programme, la théorie derrière le filtre NLMS, et des photos de la parabole, consultez l'article dédié sur mon blog : [Temps réel : Comment bien espionner vos voisins ?](https://selvasystems.net/temps-reel-comment-bien-espionner-vos-voisins-parabole-acoustique-et-filtre-nlms).

## Licence

Ce projet est sous licence MIT. Vous êtes libre de l'utiliser, de le modifier et de le distribuer, à condition de conserver cette mention de licence et de créditer les auteurs.

---

**Contributeurs :** 
Si vous avez des améliorations, idées ou questions, n'hésitez pas à ouvrir une issue ou une pull request !
