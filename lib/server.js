const { spawn } = require('child_process');
const fs = require('fs');
const net = require('net');
const shortid = require('shortid');
const express = require('express');
const bodyParser = require('body-parser');

const platforms = {
    LINUX: Symbol("LINUX"),
    MACOS: Symbol("MAC"),
};

function getPlatform() {
    if (process.platform.includes("linux")) {
        return platforms.LINUX;
    } else if (process.platform.includes("darwin")) {
        return platforms.MACOS;
    } else {
        throw new Error('platform is not supported: ' + process.platform);
    }
}


class SandsServer {
    constructor(address, port) {
        this.address = address;
        this.port = port;
        this.children = {};
        this.sockets = {};
    }

    init() {
        const app = express();
        app.use(bodyParser.json());
        app.use(bodyParser.urlencoded({ extended: true }));
        app.post('/spawn', (req, res) => {
            const {env, args, command, filename} = req.body;
            const {id, sockaddr} = this._spawnProcess(command, args, env, filename);
            res.send({id, sockaddr});
        });
        app.post('/change/:id', async (req, res) => {
            const {alpha, seconds, nanoseconds} = req.body;
            await this._changeTime(req.params.id, alpha, seconds, nanoseconds);
            res.send({result: 'OK'});
        });
        app.listen(this.port, this.address);
    }

    _spawnProcess(command, args, env, filename) {
        const platform = getPlatform();
        if (platform === platforms.LINUX) {
            env.LD_PRELOAD = './lib.so'; // todo fix this
        } else if (platform === platforms.MACOS) {
            env.DYLD_INSERT_LIBRARIES = '../lib.so'; // todo fix this
            env.DYLD_FORCE_FLAT_NAMESPACE = 'y';
        }

        const sockid = shortid.generate();
        const sockaddr = `/tmp/sand${sockid}.sock`;

        env.SANDS_SUN = sockaddr;

        const server = net.createServer();
        server.listen(sockaddr);
        server.on('connection', (socket) => {
            this.sockets[sockid] = socket;
        });

        const child = spawn(command, args, {env});
        const stream = fs.createWriteStream(filename);
        child.stdout.pipe(stream, {end: false});
        child.stderr.pipe(stream, {end: false});
        process.stdin.pipe(child.stdin);
        this.children[sockid] = child;
        child.on('error', () => stream.close());
        child.on('exit', () => stream.close());
    
        return { sockaddr, id: sockid, child };
    }

    async _changeTime(id, alpha, seconds, nanoseconds) {
        const child = this.children[id];
        const childSocket = this.sockets[id];
        
        if (childSocket == null) {
            throw new Error(`The process ${child.pid} has not connected yet`);
        }

        childSocket.write(`${alpha}:${seconds}:${nanoseconds}\n`);
        child.kill('SIGUSR2');
    }
}

module.exports = SandsServer;

