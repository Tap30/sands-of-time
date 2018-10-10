const Promise = require('bluebird');
const { spawn } = require('child_process');
const fs = require('fs');
const net = require('net');
const path = require('path');
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
        this.processesTimeSpecs = {};
        this.sockets = {};
        this.app = express();
        this.server = null;
    }

    async init() {
        this.app = express();
        this.app.use(bodyParser.json());
        this.app.use(bodyParser.urlencoded({ extended: true }));
        this.app.post('/spawn', (req, res) => {
            const { env, args, command, filename } = req.body;
            const { id, sockaddr } = this._spawnProcess(command, args, env, filename);
            res.send({ id, sockaddr });
        });
        this.app.post('/change/:id', async (req, res) => {
            const { alpha, seconds, nanoseconds } = req.body;
            await this._changeTime(req.params.id, alpha, seconds, nanoseconds);
            res.send({});
        });
        this.app.post('/shift/:id', async (req, res) => {
            const {alpha, seconds, nanoseconds} = req.body;
            const {id} = req.params;
            const d = new Date().getTime();
            const timeSpecs = this.processesTimeSpecs[id];
            let time = (d - timeSpecs.t0) * timeSpecs.alpha + timeSpecs.beta;
            time += seconds * 1000 + nanoseconds * 1e-6;
            const newSeconds = Math.floor(time / 1000);
            const newNanoseconds = Math.floor((time - newSeconds * 1000) * 1e+6);
            await this._changeTime(id, alpha, newSeconds, newNanoseconds);
            res.send({ time });
        });
        return Promise.fromCallback(cb => {
            this.server = this.app.listen(this.port, cb);
        });
    }

    async close() {
        this.childrenIds = Object.keys(this.children);
        this.childrenIds.forEach(id => {
            this.children[id].kill('SIGINT');
        });
        this.server.close();
    }

    _spawnProcess(command, args, env, filename) {
        const platform = getPlatform();
        if (platform === platforms.LINUX) {
            env.LD_PRELOAD = path.join(__dirname, '../builds/lib.so').toString();
        } else if (platform === platforms.MACOS) {
            env.DYLD_INSERT_LIBRARIES = path.join(__dirname, '../builds/lib.so').toString(); 
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
        const child = spawn(command, args, { env });
        const stream = fs.createWriteStream(filename);
        child.stdout.pipe(stream, { end: false });
        child.stderr.pipe(stream, { end: false });
        process.stdin.pipe(child.stdin);
        this.children[sockid] = child;
        this.processesTimeSpecs[sockid] = {
            alpha: 1,
            beta: new Date().getTime(),
            t0: new Date().getTime(),
        };
        child.on('error', () => stream.close());
        child.on('exit', () => stream.close());

        return { sockaddr, id: sockid, child };
    }

    async _changeTime(id, alpha, seconds, nanoseconds) {
        this.processesTimeSpecs[id] = {
            alpha,
            beta: seconds * 1000 + nanoseconds * 1e-6,
            t0: new Date().getTime(),
        };

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

