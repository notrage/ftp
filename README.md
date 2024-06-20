# File Transfer Protocol
This is a university project (in collaboration with [Axel Deleuze-Dordron](https://github.com/Stonksmen)) that aim to create a file transfer server and client that could recieve them using the network.
## Installation
1. Clone this git repository :
```bash
git clone git@github.com:notrage/ftp.git
```
2. Navigate to the project directory:
```bash
cd ftp
```
3. Build the project :
```bash
make
```
4. Run the server application :
```bash
./server_app/server
```
5. Run a client application in another terminal :
```bash
./client_app/client <server_ip>
```
6. Type in the client terminal the name of file that you want to download from the server storage
```bash
<file_name>
```
For now, it is possible to connect 3 independant clients to the server, but this number can be increased by modifying the `NB_PROC` constant in the `server.c` file.