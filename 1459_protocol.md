# Spécification du Protocole IRC (RFC 1459) — Projet ft_irc (42)

Ce document rassemble et synthétise les parties essentielles de la [RFC 1459](https://tools.ietf.org/html/rfc1459) nécessaires pour l'implémentation du serveur IRC dans le cadre du projet **ft_irc** de l'École 42.

> **Note importante :** Le sujet de 42 simplifie considérablement la RFC 1459. La gestion des communications inter-serveurs (*server-to-server*) et la propagation des réseaux sont exclues. Votre serveur est un serveur autonome (*standalone*) gérant uniquement des connexions directes de clients TCP.

---

## 1. Principes Réseau & Architecture Client-Serveur

### 1.1 Exigences Techniques (Sujet 42)
* **Sockets Non-Bloquantes :** Utilisation de `fcntl(fd, F_SETFL, O_NONBLOCK)` sur toutes les sockets (écoute et clients).
* **Multiplexage I/O :** Un seul appel système de multiplexage (`poll()`, ou `epoll`/`kqueue` selon les règles spécifiques de votre campus) doit gérer toutes les opérations de lecture et d'écriture sans bloquer le serveur.
* **Protocole de Transport :** TCP/IP (IPv4 / IPv6).

### 1.2 Format des Messages IRC
Tous les messages échangés entre les clients et le serveur sont en texte brut.

* **Délimiteur de fin de ligne :** Chaque message DOIT se terminer impérativement par `<CR><LF>` (`\r\n`).
* **Taille Maximale :** Un message ne doit pas dépasser **512 caractères** au total (incluant le `\r\n`).

### 1.3 Syntaxe Générale d'un Message
```text
[<prefix>] <command> <params> [:<trailing>]
```

| Élément | Description | Exemple / Remarques |
| :--- | :--- | :--- |
| **`prefix`** *(Optionnel)* | Commencé par `:`. Utilisé par le serveur pour indiquer l'origine d'un message. | `:alice!u@host` ou `:srv.42.fr` |
| **`command`** | Commande textuelle en majuscules ou code numérique à 3 chiffres. | `JOIN`, `PRIVMSG`, `001` |
| **`params`** | Paramètres séparés par des espaces (maximum 15 paramètres au total). | `#canal_42` |
| **`trailing`** *(Optionnel)* | Le dernier paramètre, précédé de `:`. Il est le seul à pouvoir contenir des espaces. | `:Bonjour à tous !` |

---

## 2. Processus d'Enregistrement de la Connexion

Un client est considéré comme **incomplet** tant qu'il n'a pas validé les étapes d'enregistrement. Il ne doit avoir accès à aucune autre commande (comme `JOIN` ou `PRIVMSG`) avant la fin de ce processus.

### 2.1 Séquence d'Authentification
1. **`PASS <password>`** : Vérification du mot de passe de connexion au serveur (si un mot de passe a été défini au lancement de l'exécutable).
2. **`NICK <nickname>`** : Choix du pseudonyme (doit être unique sur le serveur).
3. **`USER <username> <hostname> <servername> <realname>`** : Transmission des informations utilisateur (`<realname>` est souvent transmis dans le *trailing*).

### 2.2 Finalisation de l'Enregistrement
Lorsque le client a fourni un `PASS` valide, un `NICK` valide et unique, ainsi qu'un `USER`, le serveur doit lui envoyer le message d'accueil officiel :

```text
:server_name 001 <nickname> :Welcome to the Internet Relay Network <nickname>!<username>@<hostname>
```

---

## 3. Commandes Obligatoires

### 3.1 Gestion de la Connexion & Session
* **`PASS <password>`**
  * *Rôle :* Définit le mot de passe de connexion.
  * *Erreurs fréquentes :* `461` (ERR_NEEDMOREPARAMS), `462` (ERR_ALREADYREGISTRED), `464` (ERR_PASSWDMISMATCH).

* **`NICK <nickname>`**
  * *Rôle :* Définit ou change le pseudonyme d'un utilisateur.
  * *Erreurs fréquentes :* `431` (ERR_NONICKNAMEGIVEN), `432` (ERR_ERRONEUSNICKNAME), `433` (ERR_NICKNAMEINUSE).

* **`USER <username> <hostname> <servername> :<realname>`**
  * *Rôle :* Spécifie le nom d'utilisateur et le nom réel du client.
  * *Erreurs fréquentes :* `461` (ERR_NEEDMOREPARAMS), `462` (ERR_ALREADYREGISTRED).

* **`PING <server1> [<server2>]` / `PONG <server1> [<server2>]`**
  * *Rôle :* Maintient la connexion active et vérifie que l'autre partie réagit toujours.
  * *Comportement :* Lorsqu'un client envoie `PING :token`, le serveur doit répondre immédiatement `PONG :token`.

* **`QUIT [:<reason>]`**
  * *Rôle :* Interrompt la session du client.
  * *Comportement :* Le serveur envoie un message de départ à tous les utilisateurs partageant un ou plusieurs canaux avec ce client, puis ferme la socket.

---

### 3.2 Communications & Messagerie
* **`PRIVMSG <target> :<message>`**
  * *Rôle :* Envoie un message texte à un destinataire (un autre utilisateur ou un canal complet).
  * *Cible utilisateur :* Envoyé directement au client visé.
  * *Cible canal :* Diffusé à tous les membres du canal **sauf** à l'expéditeur.
  * *Erreurs fréquentes :* `401` (ERR_NOSUCHNICK), `404` (ERR_CANNOTSENDTOCHAN), `411` (ERR_NORECIPIENT), `412` (ERR_NOTEXTTOSEND).

* **`NOTICE <target> :<message>`**
  * *Rôle :* Fonctionne exactement comme `PRIVMSG`, à une exception majeure près : **il ne doit jamais générer de réponse d'erreur automatique (numeric replies)** pour éviter les boucles d'erreurs entre bots ou scripts.

---

### 3.3 Gestion des Canaux (Channels)
Les noms de canaux doivent impérativement commencer par le caractère `#` (ou `&`).

* **`JOIN <channel>{,<channel>} [<key>{,<key>]`**
  * *Rôle :* Permet de rejoindre un ou plusieurs canaux. Si le canal n'existe pas, le serveur le crée et attribue automatiquement le statut d'**Opérateur de canal** à cet utilisateur.
  * *Réponses associées :* `332` (RPL_TOPIC), `353` (RPL_NAMREPLY), `366` (RPL_ENDOFNAMES).
  * *Erreurs fréquentes :* `471` (ERR_CHANNELISFULL), `473` (ERR_INVITEONLYCHAN), `475` (ERR_BADCHANNELKEY).

* **`PART <channel>{,<channel>} [:<reason>]`**
  * *Rôle :* Quitte un ou plusieurs canaux spécifiés.
  * *Erreurs fréquentes :* `403` (ERR_NOSUCHCHANNEL), `442` (ERR_NOTONCHANNEL).

* **`TOPIC <channel> [:<topic>]`**
  * *Rôle :* Consulter ou modifier le sujet d'un canal.
  * *Si `<topic>` est omis :* Renvoie le sujet actuel du canal (`332`) ou indique qu'il n'y en a pas (`331`).
  * *Si `<topic>` est fourni :* Modifie le sujet (soumis aux restrictions du mode `+t`).
  * *Erreurs fréquentes :* `442` (ERR_NOTONCHANNEL), `482` (ERR_CHANOPRIVSNEEDED).

* **`NAMES [<channel>{,<channel>}]`**
  * *Rôle :* Liste l'ensemble des utilisateurs présents sur le ou les canaux indiqués.

---

### 3.4 Commandes d'Administration & Opérateurs
Ces commandes nécessitent que l'émetteur possède les droits d'**Opérateur du canal** (`ChanOp`).

* **`KICK <channel> <user> [:<comment>]`**
  * *Rôle :* Expulse un utilisateur d'un canal.
  * *Erreurs fréquentes :* `401` (ERR_NOSUCHNICK), `403` (ERR_NOSUCHCHANNEL), `441` (ERR_USERNOTINCHANNEL), `442` (ERR_NOTONCHANNEL), `482` (ERR_CHANOPRIVSNEEDED).

* **`INVITE <nickname> <channel>`**
  * *Rôle :* Invite un utilisateur à rejoindre un canal (nécessaire si le canal est configuré en mode invitation `+i`).
  * *Erreurs fréquentes :* `401` (ERR_NOSUCHNICK), `442` (ERR_NOTONCHANNEL), `443` (ERR_USERONCHANNEL), `482` (ERR_CHANOPRIVSNEEDED).

* **`MODE <target> [<modestring> [params]]`**
  * *Rôle :* Configure les paramètres et permissions d'un canal ou d'un utilisateur.
  * **Modes de canal obligatoires (selon barème 42) :**
    * **`i`** : Canal sur invitation uniquement (*Invite-only*).
    * **`t`** : Restriction de la modification du `TOPIC` aux seuls opérateurs du canal.
    * **`k`** : Définition ou suppression du mot de passe (*Key*) du canal (`MODE #chan +k password`).
    * **`o`** : Attribution ou retrait du privilège d'opérateur à un utilisateur (`MODE #chan +o alice`).
    * **`l`** : Définition ou suppression de la limite maximale d'utilisateurs autorisés (`MODE #chan +l 10`).

---

## 4. Table des Réponses Numériques (Numeric Replies)

Les réponses numériques sont structurées sous le format : `:server_name <3-digit-code> <target_nick> <params>`

### 4.1 Réponses Succès / Information (001 - 399)
| Code | Nom RFC | Description / Message standard |
| :--- | :--- | :--- |
| **`001`** | `RPL_WELCOME` | `:Welcome to the Internet Relay Network <nick>!<user>@<host>` |
| **`331`** | `RPL_NOTOPIC` | `<channel> :No topic is set` |
| **`332`** | `RPL_TOPIC` | `<channel> :<topic>` |
| **`341`** | `RPL_INVITING` | `<channel> <nick>` |
| **`353`** | `RPL_NAMREPLY` | `( "=" / "*" / "@" ) <channel> :[PREFIX]<nick1> [PREFIX]<nick2> ...` |
| **`366`** | `RPL_ENDOFNAMES` | `<channel> :End of /NAMES list` |

### 4.2 Messages d'Erreur (400 - 599)
| Code | Nom RFC | Cause / Message standard |
| :--- | :--- | :--- |
| **`401`** | `ERR_NOSUCHNICK` | `<nickname> :No such nick/channel` |
| **`403`** | `ERR_NOSUCHCHANNEL` | `<channel_name> :No such channel` |
| **`404`** | `ERR_CANNOTSENDTOCHAN` | `<channel_name> :Cannot send to channel` |
| **`411`** | `ERR_NORECIPIENT` | `:No recipient given (<command>)` |
| **`412`** | `ERR_NOTEXTTOSEND` | `:No text to send` |
| **`431`** | `ERR_NONICKNAMEGIVEN` | `:No nickname given` |
| **`432`** | `ERR_ERRONEUSNICKNAME` | `<nick> :Erroneous nickname` |
| **`433`** | `ERR_NICKNAMEINUSE` | `<nick> :Nickname is already in use` |
| **`441`** | `ERR_USERNOTINCHANNEL` | `<nick> <channel> :They aren't on that channel` |
| **`442`** | `ERR_NOTONCHANNEL` | `<channel> :You're not on that channel` |
| **`443`** | `ERR_USERONCHANNEL` | `<user> <channel> :is already on channel` |
| **`461`** | `ERR_NEEDMOREPARAMS` | `<command> :Not enough parameters` |
| **`462`** | `ERR_ALREADYREGISTRED` | `:Unauthorized command (already registered)` |
| **`464`** | `ERR_PASSWDMISMATCH` | `:Password incorrect` |
| **`471`** | `ERR_CHANNELISFULL` | `<channel> :Cannot join channel (+l)` |
| **`473`** | `ERR_INVITEONLYCHAN` | `<channel> :Cannot join channel (+i)` |
| **`475`** | `ERR_BADCHANNELKEY` | `<channel> :Cannot join channel (+k)` |
| **`482`** | `ERR_CHANOPRIVSNEEDED` | `<channel> :You're not channel operator` |

---

## 5. Astuces de Parsing & Implémentation

1. **Buffer de Réception (TCP Framing) :** Les données TCP arrivent sous forme de flux (*stream*). Un paquet reçu via `recv()` peut contenir un message incomplet ou plusieurs messages collés. Il faut toujours accumuler les données reçues dans un buffer par client et découper la chaîne sur la séquence `\r\n`.
2. **Gestion du Trailing :** Si un argument commence par `:`, tous les mots qui suivent (même s'ils contiennent des espaces) forment un seul et unique paramètre jusqu'au `\r\n` final.
3. **Cas de NICK en cours de session :** Modifier son pseudo (`NICK`) lorsqu'on est déjà connecté et présent sur des canaux doit générer une notification à destination de tous les membres des canaux partagés.