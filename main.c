#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096

const char* WORDS[] = {
    "армия", "атака", "барин", "ветер", "ветка", "видео", "война", "ворон",
    "кабан", "каток", "кокос", "лимит", "медик", "океан", "палка", "песня",
    "петух", "радио", "редис", "ручка", "слава", "слово", "тайга", "театр"
};
const int WORD_COUNT = 24;

typedef struct {
    char target[6];
    int gameWon;
    int gameOver;
    char guesses[6][6];
    int guessCount;
} GameState;

GameState games[100];
int gameCounter = 0;

void send_response(int client_socket, char *content_type, char *body) {
    char response[BUFFER_SIZE];
    sprintf(response, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        content_type, strlen(body), body);
    send(client_socket, response, strlen(response), 0);
}

void serve_html(int client_socket) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><meta charset='UTF-8'><title>Wordle</title>"
        "<style>"
        "body{font-family:Arial;text-align:center;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);padding:20px}"
        ".container{background:white;border-radius:20px;padding:30px;max-width:500px;margin:0 auto}"
        ".tile{width:60px;height:60px;border:2px solid #d3d6da;display:inline-flex;margin:5px;align-items:center;justify-content:center;font-size:2rem;font-weight:bold}"
        ".green{background:#6aaa64;color:white;border-color:#6aaa64}"
        ".yellow{background:#c9b458;color:white;border-color:#c9b458}"
        ".gray{background:#787c7e;color:white;border-color:#787c7e}"
        ".keyboard{margin-top:20px}"
        ".key{background:#d3d6da;border:none;padding:15px;margin:3px;border-radius:8px;cursor:pointer;font-size:1rem;font-weight:bold}"
        ".key:hover{background:#bbb}"
        ".message{padding:15px;margin-top:20px;border-radius:10px}"
        ".success{background:#d4edda;color:#155724}"
        ".error{background:#f8d7da;color:#721c24}"
        "button{background:#4CAF50;color:white;border:none;padding:12px 30px;border-radius:25px;cursor:pointer;margin-top:20px}"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>🎮 РУССКИЙ WORDLE</h1>"
        "<div id='board'></div>"
        "<div class='keyboard' id='keyboard'></div>"
        "<button onclick='newGame()'>🔄 Новая игра</button>"
        "<div id='message'></div>"
        "</div>"
        "<script>"
        "let currentGuess='',guesses=[],gameWon=false,gameOver=false,currentRow=0;"
        "const rows=['йцукенгшщзхъ','фывапролджэ','ячсмитьбюё'];"
        "function initKeyboard(){"
        "let kb=document.getElementById('keyboard');kb.innerHTML='';"
        "rows.forEach(row=>{let div=document.createElement('div');"
        "row.split('').forEach(l=>{let b=document.createElement('button');"
        "b.className='key';b.textContent=l.toUpperCase();"
        "b.onclick=()=>handleKey(l);div.appendChild(b);});kb.appendChild(div);});"
        "let div=document.createElement('div');"
        "let enter=document.createElement('button');enter.className='key';enter.textContent='↵';enter.onclick=submitGuess;"
        "let back=document.createElement('button');back.className='key';back.textContent='⌫';back.onclick=deleteLetter;"
        "div.appendChild(enter);div.appendChild(back);kb.appendChild(div);}"
        "function handleKey(l){if(gameWon||gameOver||currentGuess.length>=5)return;currentGuess+=l;updateBoard();}"
        "function deleteLetter(){if(gameWon||gameOver)return;currentGuess=currentGuess.slice(0,-1);updateBoard();}"
        "async function submitGuess(){if(gameWon||gameOver||currentGuess.length!=5)return;"
        "let r=await fetch('/game',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({word:currentGuess})});"
        "let d=await r.json();if(d.status=='ok'){"
        "let res=d.result.split(',');"
        "let tiles=document.querySelectorAll('.row')[currentRow].querySelectorAll('.tile');"
        "for(let i=0;i<5;i++){tiles[i].textContent=currentGuess[i].toUpperCase();"
        "if(res[i]=='2')tiles[i].classList.add('green');"
        "else if(res[i]=='1')tiles[i].classList.add('yellow');"
        "else tiles[i].classList.add('gray');}"
        "guesses[currentRow]=currentGuess;currentRow++;currentGuess='';"
        "if(d.won){gameWon=true;showMessage('🎉 ПОБЕДА!','success');}"
        "else if(d.gameOver){gameOver=true;showMessage('😔 Проигрыш! Слово: '+d.target.toUpperCase(),'error');}"
        "updateBoard();}}"
        "function updateBoard(){let b=document.getElementById('board');b.innerHTML='';"
        "for(let i=0;i<6;i++){let row=document.createElement('div');row.className='row';"
        "for(let j=0;j<5;j++){let t=document.createElement('div');t.className='tile';"
        "if(i<currentRow&&guesses[i])t.textContent=guesses[i][j].toUpperCase();"
        "else if(i==currentRow&&j<currentGuess.length)t.textContent=currentGuess[j].toUpperCase();"
        "row.appendChild(t);}b.appendChild(row);}}"
        "function showMessage(msg,type){let m=document.getElementById('message');m.className='message '+type;m.textContent=msg;setTimeout(()=>m.style.display='none',3000);m.style.display='block';}"
        "async function newGame(){let r=await fetch('/game',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({new_game:true})});"
        "let d=await r.json();if(d.status=='ok'){currentGuess='';guesses=[];gameWon=false;gameOver=false;currentRow=0;updateBoard();showMessage('Новая игра!','success');}}"
        "initKeyboard();updateBoard();newGame();"
        "</script>"
        "</body></html>";
    
    send_response(client_socket, "text/html", (char*)html);
}

void handle_game_request(int client_socket, char *request_body) {
    GameState *game = &games[gameCounter - 1];
    char response[BUFFER_SIZE];
    
    if (strstr(request_body, "new_game")) {
        int idx = rand() % WORD_COUNT;
        strcpy(game->target, WORDS[idx]);
        game->gameWon = 0;
        game->gameOver = 0;
        game->guessCount = 0;
        sprintf(response, "{\"status\":\"ok\"}");
        send_response(client_socket, "application/json", response);
        return;
    }
    
    char guess[10] = "";
    char *ws = strstr(request_body, "word\":\"");
    if (ws) {
        ws += 7;
        int i = 0;
        while (ws[i] != '\"' && i < 5) {
            guess[i] = ws[i];
            i++;
        }
        guess[i] = '\0';
    }
    
    if (strlen(guess) == 5 && !game->gameWon && game->guessCount < 6) {
        strcpy(game->guesses[game->guessCount], guess);
        game->guessCount++;
        
        int isWin = (strcmp(guess, game->target) == 0);
        if (isWin || game->guessCount >= 6) {
            game->gameWon = isWin;
            game->gameOver = 1;
        }
        
        char result[20] = "";
        for (int i = 0; i < 5; i++) {
            if (guess[i] == game->target[i]) strcat(result, "2");
            else if (strchr(game->target, guess[i])) strcat(result, "1");
            else strcat(result, "0");
            if (i < 4) strcat(result, ",");
        }
        
        sprintf(response,
            "{\"status\":\"ok\",\"won\":%d,\"gameOver\":%d,\"result\":\"%s\",\"target\":\"%s\"}",
            isWin, game->gameOver, result, game->gameOver ? game->target : "");
        send_response(client_socket, "application/json", response);
    }
}

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    char method[10], path[100];
    sscanf(buffer, "%s %s", method, path);
    
    if (strcmp(path, "/") == 0) {
        serve_html(client_socket);
    } else if (strcmp(path, "/game") == 0 && strcmp(method, "POST") == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) handle_game_request(client_socket, body + 4);
    }
    close(client_socket);
}

int main() {
    srand(time(NULL));
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("Server running on port %d\n", PORT);
    
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        gameCounter++;
        handle_request(client_socket);
    }
    return 0;
}
