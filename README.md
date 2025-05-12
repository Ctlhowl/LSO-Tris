# Tris - Server multi-client per giocare a Tris

![Docker](https://img.shields.io/badge/Docker-2CA5E0?style=for-the-badge&logo=docker&logoColor=white) ![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white) ![C](https://img.shields.io/badge/C-A8B9CC?style=for-the-badge&logo=c&logoColor=black)

Tecnologie usate:
- **C server** (gestisce la logica del gioco)
- **Python/Pygame client** (interfaccia grafica)
- **Docker Compose** per il deployment

## 📋 Requisiti 
- Docker Engine - Community ([Install guide](https://docs.docker.com/engine/install/))
- Docker Compose v2 (verifica la versione con `docker compose version`)
- X11 server (per la GUI su Linux)
 
**Nota per Linux:**
Abilita X11 forwarding prima di avviare:
```bash
xhost +local:docker
echo $DISPLAY
```
Dovresti vedere `:0` o simile (es. `:1`)

**Nota per Windows:**
1. **Installare VcXsrv (Server X)**
	1. **Scarica e installa VcXsrv:**
		- Vai su https://sourceforge.net/projects/vcxsrv/ e scarica l'ultima versione di VcXsrv.
	    - Installa il programma sul tuo sistema.
	2. **Avvia VcXsrv:**
		- Dopo l'installazione, avvia **VcXsrv**.
		- Ti verrà chiesto di configurare alcune opzioni. Segui questi passi:
			- Seleziona **Multiple windows** per avere finestre separate per ogni applicazione.
			- Abilita l'opzione **Start no client** per non dover usare un'applicazione client separata.
			- Scegli **Disable access control**.
			- Lascia le altre impostazioni predefinite e premi **Avanti** fino a completare la configurazione

## 🚀 Avvio Rapido
```bash

# Clona la repository
git clone git@github.com:Ctlhowl/LSO-Tris.git
cd LSO-Tris

# Build ed esecuzione di docker compose
docker compose up --build
```

> Verranno avviati:
> 
> - **1 server** (in ascolto su `localhost:8080`)
>     
> - **1 client** (finestra Pygame)


```bash
docker compose up --build --scale client=3
```
> Verranno avviati:
> 
> - **1 server** (in ascolto su `localhost:8080`)
>     
> - **3 client** (finestra Pygame)

## 🛠️ Comandi Utili
|Comando|Descrizione|
|---|---|
|`docker compose up -d server`|Avvia solo il server in background|
|`docker compose run client`|Avvia un client aggiuntivo manualmente|
|`docker compose logs -f server`|Monitora i log del server|
|`docker compose down --volumes`|Ferma tutto e cancella i dati|
