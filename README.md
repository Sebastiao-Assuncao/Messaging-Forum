# Messaging Forum

## Context
Centralized messaging service provided by central "Directory Server" and various "Users" operating on different machines connected to the internet

## Usage
./user [-n DSIP] [-p DSport]\
./DS [-p DSport] [-v]

## Available User Commands
- reg UID pass
- unregister UID pass
- login UID pass
- logout
- showuid or su
- exit
- groups or gl
- subscribe GID GName or s GID GName
- unsubscribe GID or u GID
- my_groups or mgl
- select GID or sag GID
- showgid or sg
- ulist or ul
- post “text” [Fname]
- retrieve MID or r MID