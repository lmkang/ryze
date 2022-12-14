const http = require('http');
const cluster = require('cluster');

if(cluster.isMaster) {
    for(let i = 0; i < 4; i++) {
        cluster.fork();
    }
} else {
    http.createServer((req, res) => {
        res.end('hello world!');
    }).listen(9898);
}
