# ft_irc : comprendre le projet

Ce guide explique **ce que tu construis** et **pourquoi** chaque contrainte du sujet existe. À lire avant d'ouvrir ton éditeur.

---

## 1. Le projet en une phrase

Tu écris un **serveur** IRC : un programme qui attend des connexions TCP, lit des lignes de texte envoyées par des clients (HexChat, irssi...) et leur répond selon le protocole IRC. Tu n'écris **pas** de client, et ton serveur ne parle pas à d'autres serveurs.

```
 [Client A] ──┐
 [Client B] ──┼──► [ ircserv ] ◄── tu codes ça
 [Client C] ──┘
```

## 2. Le protocole IRC

IRC est un protocole **texte**. Chaque message est une ligne terminée par `\r\n` (512 octets max) :

```
[:prefix] COMMAND param1 param2 ... [:trailing]
```

Exemples :

| Ligne | Sens |
|---|---|
| `PASS secret` | le client donne le mot de passe |
| `NICK alice` | il choisit son pseudo |
| `USER alice 0 * :Alice Martin` | il donne son username et son nom réel |
| `JOIN #general` | il rejoint un channel |
| `PRIVMSG #general :salut` | il envoie un message au channel |
| `PRIVMSG bob :salut` | il envoie un message privé à bob |

- Le **trailing** (après `:`) est le seul paramètre qui peut contenir des espaces.
- Le **prefix** est ce que *ton serveur* ajoute quand il relaie un message : `:alice!alice@host PRIVMSG #general :salut`.
- Les **réponses du serveur** sont des codes numériques : `001` bienvenue, `433` pseudo déjà pris, `461` pas assez de paramètres, etc.

## 3. Les notions du serveur IRC

### Client
Une connexion TCP + un état : pas encore authentifié → mot de passe OK → pseudo et username donnés → **enregistré**. Tant qu'il n'est pas enregistré, il ne peut presque rien faire.

### Channel
Un salon commençant par `#`. Il contient des membres, un topic, des **opérateurs**, et des **modes** :

| Mode | Effet |
|---|---|
| `i` | invite-only : il faut être invité pour entrer |
| `t` | seul un opérateur peut changer le topic |
| `k` | le channel a un mot de passe (clé) |
| `o` | donne/retire le statut d'opérateur à un membre |
| `l` | limite le nombre de membres |

### Opérateur
Le premier à rejoindre un channel qui n'existe pas le crée et devient opérateur. Seuls les opérateurs peuvent `KICK`, `INVITE`, `MODE` (et `TOPIC` si `+t`).

### Diffusion
Un message envoyé à un channel doit être relayé à **tous les autres membres** (pas à l'émetteur).

## 4. Les concepts réseau

### Socket d'écoute vs socket client
- La **socket d'écoute** (`socket` → `bind` → `listen`) ne sert qu'à recevoir les nouvelles connexions.
- À chaque `accept()`, tu obtiens un **nouveau fd** : celui de ce client. C'est avec lui que tu lis et écris.

### TCP est un flux, pas des messages
Point le plus important du projet. TCP ne garantit **pas** qu'un `recv()` te rende une commande entière :

```
Le client envoie : "PRIVMSG #a :hello\r\n"
Ton recv() peut lire : "PRIV"  puis  "MSG #a :he"  puis  "llo\r\n"
```

Ou bien **plusieurs commandes d'un coup** : `"NICK a\r\nUSER a 0 * :A\r\n"`.

Donc :
1. chaque client a un **buffer d'entrée** ;
2. à chaque `recv()`, tu **ajoutes** au buffer ;
3. tant que le buffer contient un `\n`, tu extrais la ligne et tu la traites ;
4. le reste attend la suite.

C'est ce que teste le `nc` + `Ctrl+D` du sujet.

### Non-bloquant
Un `recv()` ou `send()` classique peut **bloquer** (attendre indéfiniment). Avec un fd non bloquant, il rend la main immédiatement. Tu l'actives avec `fcntl(fd, F_SETFL, O_NONBLOCK)`.

### Multiplexage avec `poll()`
Sans fork ni threads, comment gérer 50 clients ? Avec **un seul `poll()`** qui surveille tous les fds en même temps et te dit lesquels sont prêts :

```
boucle infinie :
    poll(tous les fds)              ← attend qu'il se passe quelque chose
    pour chaque fd prêt :
        si socket d'écoute  → accept()
        si POLLIN client    → recv() puis traiter les lignes complètes
        si POLLOUT client   → send() ce qui est en attente
        si POLLHUP/POLLERR  → déconnecter
```

### Pourquoi un buffer de sortie ?
Un `send()` peut n'envoyer qu'une partie des octets (réseau lent, buffer plein). Tu stockes donc les réponses dans un **buffer de sortie** par client, tu demandes `POLLOUT` uniquement quand il n'est pas vide, et tu retires du buffer ce qui a vraiment été envoyé.

## 5. Pourquoi les règles du sujet existent

| Règle | Raison |
|---|---|
| Pas de `fork` | tout se fait dans un seul processus, piloté par `poll()` |
| Un seul `poll()` | un point central pour lire, écrire et écouter |
| `recv`/`send` seulement après `poll()` | sinon tu fais du *busy-waiting* qui gaspille le CPU (**note 0**) |
| Ne pas se fier à `errno` | la décision doit venir de `poll()` et de la valeur de retour (**note 0**) |
| Aucun crash | un seul client malveillant ou bizarre ne doit pas faire tomber le serveur (**note 0**) |
| C++98 | pas de `auto`, pas de `nullptr`, pas de `std::to_string`, pas de range-for |

## 6. Cycle de vie d'un client

```
connexion TCP (accept)
      │
      ▼
 PASS ─► NICK ─► USER        (dans n'importe quel ordre pour NICK/USER,
      │                       mais PASS d'abord)
      ▼
 enregistré → le serveur envoie 001..004 (bienvenue)
      │
      ▼
 JOIN / PRIVMSG / PART / KICK / MODE / TOPIC / INVITE ...
      │
      ▼
 QUIT ou coupure réseau → retirer le client de tous les channels,
                          fermer le fd, libérer la mémoire
```

## 7. Exemple complet d'échange

```
Client → PASS secret
Client → NICK alice
Client → USER alice 0 * :Alice
Serveur → :ircserv 001 alice :Welcome to the IRC network alice

Client → JOIN #general
Serveur → :alice!alice@host JOIN #general
Serveur → :ircserv 353 alice = #general :@alice
Serveur → :ircserv 366 alice #general :End of /NAMES list

(bob rejoint, puis écrit)
Bob → PRIVMSG #general :salut alice
Alice reçoit → :bob!bob@host PRIVMSG #general :salut alice
```

Le `@` devant `alice` dans `353` indique qu'elle est opérateur.

## 8. Les erreurs courantes à connaître

| Code | Nom | Quand |
|---|---|---|
| 401 | ERR_NOSUCHNICK | pseudo inconnu |
| 403 | ERR_NOSUCHCHANNEL | channel inexistant |
| 404 | ERR_CANNOTSENDTOCHAN | impossible d'écrire dans le channel |
| 421 | ERR_UNKNOWNCOMMAND | commande inconnue |
| 431 / 432 / 433 | ERR_NONICKNAMEGIVEN / ERRONEUSNICKNAME / NICKNAMEINUSE | problèmes de pseudo |
| 441 / 442 | ERR_USERNOTINCHANNEL / NOTONCHANNEL | cible ou émetteur absent du channel |
| 451 | ERR_NOTREGISTERED | commande avant enregistrement |
| 461 | ERR_NEEDMOREPARAMS | paramètres manquants |
| 462 | ERR_ALREADYREGISTRED | `USER`/`PASS` déjà faits |
| 464 | ERR_PASSWDMISMATCH | mauvais mot de passe |
| 471 / 473 / 474 / 475 | CHANNELISFULL / INVITEONLYCHAN / BANNEDFROMCHAN / BADCHANNELKEY | refus de `JOIN` |
| 482 | ERR_CHANOPRIVSNEEDED | droits opérateur requis |


## 10. Ressources pour approfondir

- RFC 1459 et RFC 2812 (protocole)
- https://modern.ircdocs.horse/ (version moderne et lisible, très pratique pour les codes de réponse)
- Beej's Guide to Network Programming (sockets, `poll`)
- `man poll`, `man recv`, `man send`, `man accept`, `man fcntl`