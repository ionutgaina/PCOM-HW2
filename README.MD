# Aplicație client-server TCP și UDP pentru gestionarea mesajelor
## Starting point
Pentru a realiaza această temă, am început de la următoarele surse:
- [Laborator 5] (https://pcom.pages.upb.ro/labs/lab5/lecture.html)
- [Laborator 7] (https://pcom.pages.upb.ro/labs/lab7/lecture.html)

Scheletul pentru client(subscriber) și server l-am luat de la laboratorul 7, respectiv laboratorul 5 a fost folosit pentru a înțelege mai bine cum funcționează protocolul UDP.

Am folosit C++ pentru a implementa această temă deoarece am dorit să încerc ceva nou și să mă folosesc de ușurința de a lucra modular, de a folosi clase și de a folosi structuri de date deja implementate (vectori, map-uri) pentru a-mi ușura munca.

## Descrierea implementării

### Server

Serverul a fost început prin crearea parsării parametrului de comandă \<PORT\> 

Apoi creez un socket TCP și unul UDP, pe care le folosesc pentru a asculta pe portul dat ca parametru.
Aceștia sunt creați prin intermediul clasei Sockets, care conține metodele necesare pentru a crea un socket, a-l lega la un port și a-l pune în modul listen. 

Eu am folosit metoda de multiplexare prin poll, pentru a putea asculta pe ambele socketuri, stdin și pe sockeții clienților. Am adăugat în poll descriptorii pentru socketul TCP, UDP, stdin;

Logica de la server este implementată în funcția run_server, care este apelată în main. În această funcție, am un while infinit, în care primesc evenimentele de la poll și le procesez.

În preambulul funcției run_server, am un map de tipul \<string, Client_TCP*\> care conține toți clienții TCP ce au fost conectați la server. Este folosit pentru a putea trimite mesaje conform cerinței;

Client_TCP este o clasă care se află în fișierul user.cpp, care conține toate informațiile necesare despre un client TCP. Aceasta conține:
  
  - int socket (dacă acesta e -1 înseamnă că e deconctat altfel conectat)
  - map\<string, int\> topic_to_sf (map care conține toate topicurile la care clientul este abonat și dacă este sau nu abonat cu store-and-forward)
  - map\<string, vector\<string\>\> topic_to_messages (map care conține toate mesajele primite de la un anumit topic abonat cu store-and-forward după ce s-a deconctat clientul)
  - Metode care ajuta la manipularea acestor date (getters, setters, metode de adăugare, ștergere, verificare existență)

În continuare, în funcția run_server, primesc evenimentele de la poll și le procesez.

#### STDIN

Dacă evenimentul este de la stdin, vom aștepta comanda "exit", altfel vom afișa un mesaj de eroare.
Această comandă eliberează resursele, sockeții și închide serverul.

#### TCP socket

Dacă evenimentul este de la socket-ul TCP ce ascultă, vom accepta conexiunea, facem verificările că id-ul clientului nu este deja folosit și îl adăugăm în map-ul de clienți. 

Id-ul este primit de la client când se conectează cu server-ul, de asemenea când se conectează un client, se adaugă în baza serverului sau dacă acesta există deja, se verifică dacă acesta este abonat la vreun topic cu store-and-forward și dacă da, se trimit toate mesajele primite de la acel topic cât timp clientul a fost deconectat. la final îl adăugăm în poll pentru a putea primi mesaje de la acesta. Și afișăm mesajul de conectare.

#### TCP client

Dacă evenimentul este de la un client TCP, vom trata cazurile în care acesta se deconectează (atunci când la recv vom primi un mesaj de lungime 0) sau trimite un mesaj (subscribe, unsubscribe)

Subscribe și unsubscribe va fi un mesaj cu o anumită structură mai exact conform structurii (struct subscribe_packet):

  - command - care va fi "subscribe" sau "unsubscribe"
  - topic - topicul la care se abonează/dezabonează
  - id - id-ul clientului care face această acțiune (pentru a putea verifica mai ușor dacă acesta este abonat la topicul respectiv)
  - sf - dacă este sau nu abonat cu store-and-forward (acesta nu va fi considerat dacă comanda este unsubscribe)

  #### UDP socket

Dacă evenimentul este de la un client UDP, vom citi mesajul pe care acesta îl trimite.
Mesajul va avea o structură specifică, mai exact conform structurii (struct udp_packet):

  - topic - topicul la care se trimite mesajul
  - data_type - tipul de date al mesajului (INT, SHORT_REAL, FLOAT, STRING)
  - content - conținutul mesajului

După ce am citit mesajul, vom verifica dacă există clienți abonați la topicul respectiv și dacă da, vom trimite mesajul la aceștia. Dacă există clienți abonați cu store-and-forward însă deconectatți, vom adăuga mesajul în map-ul de mesaje al acelui client.

Mesajul dat de la socket-ul UDP va fi parsat cu ajutorul clasei utilitare Data_Parser (data_parser.cpp), care conține metodele necesare pentru a parsa mesajul și a-l transforma într-un string de forma:

```
<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
```

Fiecare caz de tip de date fiind tratat în parte.

### Client (subscriber)

Clientul a fost început prin crearea parsării parametrilor de comandă \<ID_CLIENT\> \<IP_SERVER\> \<PORT_SERVER\>

Apoi creez un socket TCP, pe care îl folosesc pentru a mă conecta la serverul dat ca parametru.
Am folosit metoda de multiplexare prin poll, pentru a putea asculta pe socketul TCP și pe stdin. Am adăugat în poll descriptorii pentru socketul TCP, stdin.

Logica de la client este implementată în funcția run_client, care este apelată în main. În această funcție, am un while infinit, în care primesc evenimentele de la poll și le procesez.

Când mă conectez la server, trimit un mesaj cu id-ul meu. Dacă acesta este deja folosit, voi primi un răspund de lungime 0 și voi închide clientul automat.

Când voi primi un eveniment la stdin voi verifica dacă comanda este "exit", "subscribe <topic> <sf>" sau "unsubscribe <topic>". Dacă este "exit" voi închide clientul, dacă este "subscribe" sau "unsubscribe" voi trimite un mesaj de tipul struct subscribe_packet la server.

Dacă evenimentul este de la socket-ul TCP, voi primi un mesaj de la server. Dacă acesta este de lungime 0, înseamnă că serverul s-a închis și voi închide și clientul. Dacă nu, voi afișa mesajul primit.


### Protocolul peste TCP

Am folosit un protocol destul de simplu însă eficient pentru ceea ce am nevoie:

În primul rând am o structură (struct packet) care conține:

  - header - care la rândul lui conține 2 câmpuri:
    - message_type - care poate fi 0 pentru mesajele ce sunt afișate plain text sau 1 pentru mesajele ce sunt un packet de tip subscribe_packet
    - len - lungimea mesajului pentru a putea citi payload-ul
  - content - conținutul mesajului, care poate fi plain text sau un packet de tip subscribe_packet.

În funcțiile din fișierul common.cpp (schelet de la laboratorul 7), am implementat metodele de citire și scriere a unui mesaj de tipul struct packet, care poate fi de lungime maximă de 2^16. (uint16_t).

Desigur am implementat citirea și primirea mesajelor în funcție de lungimea mesajului (dinamic), pentru a fi eficient și a nu citi mai mult decât trebuie.

Am dezactivat algoritmul de Nagle setând TCP_NODELAY pe socket


## Feedback

A fost o temă destul de înteresantă, unde chiar mi-a dat multe bătăi de cap, căci credeam că lucrez prost cu string-urile, deoarece sunt teste care verifică edge case-urile structurii mesajului UDP. Problema reală a fost că checker-ul este implementat în așa fel încât e foarte greu să-ți dai seama dacă ai făcut bine când ai o rezolvare parțială (fără unsubscribe și store-forward). Căci testam manual, rezultate erau bune, însă checker-ul îmi dădea 0 puncte. După ce am implementat și unsubscribe și store-forward (toată tema), checker-ul a început să afișeze niște rezultate după care puteam să-mi dau seama unde greșesc.

Mă bucur că am avut șansa să încerc 1000 de posibilități de a crea, copia, concatena '\0' și tot felul de chestii cu string-uri și sockeți. A început să-mi placă și mai mult C++ după această temă.

<center><h2>Mulțumesc mult pentru efortul depus.</h2></center>


