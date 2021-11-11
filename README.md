# A peer to peer file sharing system

It is a group based file sharing system where users can share, download files from the group they belongs to (only Admins will have the permissions to manage the group). Download will be parallel with multiple pieces from multiple peers.


## How to Run ?

1. Command to compile tracker : `g++ tracker.cpp -o tracker -pthread`

2. Command to compile client : `g++ client.cpp -o client -pthread -lcrypto`

3. Command to run tracker : `./tracker tracker_info.txt`

4. Command to run client : ` ./client <IP>:<PORT> tracker_info.txt`

Note : tracker_info.txt contains the ip, port details of the the tracker in the format `<ip>:<port>`

## Features
-> It can be used to download parallelly multiple pieces from multiple peers.
-> It contains only one tracker.
-> It uses SHA1 hash to check the integrity of the downloaded file.