const Promise = require('bluebird');
const request = require('request-promise');
const SandsServer = require('./server');
const path = require('path');

class Sands {
        constructor() {
                this.server = null;
                this.processIdMap = new Map();
                this.processes = [];
        }

        async runServer(address = 'http://localhost', port='9394') {
                if (this.server) {
                        throw new Error('The sands server is already running');
                }
                let sandsServer = new SandsServer(address, port);
                console.log(`Listening on port: ${port}`);
                await sandsServer.init();
                this.server = sandsServer;
                return sandsServer;
        }
        
        async killServer() {
                await this.server.close();
        }

        async spawnProcess(processName, command, args, env, logDir) {
                const filename = path.join(logDir, `${processName}.sands.log`);
                let body = {command, filename, args, env};
                body = JSON.parse(JSON.stringify(body));
                const result = await request(`${this.server.address}:${this.server.port}/spawn`, {
                        method: 'POST',
                        json: true,
                        body,
                });
                this.processIdMap.set(processName, result.id);
                this.processes.push(processName);
                return result;
        }

        _getSecondsAndNanoseconds(t) {
                const seconds = Math.floor(t / 1000);
                const nanoseconds = Math.floor((t - seconds *  1000) * 1e+6);
                return {seconds, nanoseconds};
        }

        async _sendChangeTimeRequest(processName, alpha, seconds, nanoseconds) {
                const id = this.processIdMap.get(processName);
                let body = {alpha, seconds, nanoseconds};
                body = JSON.parse(JSON.stringify(body));
                const result = await request(`${this.server.address}:${this.server.port}/change/${id}`, {
                        method: 'POST',
                        json: true,
                        body,
                });
                return result;
        }

        async _sendShiftTimeRequest(processName, alpha, seconds, nanoseconds) {
                const id = this.processIdMap.get(processName);
                let body = {alpha, seconds, nanoseconds};
                body = JSON.parse(JSON.stringify(body));
                const result = await request(`${this.server.address}:${this.server.port}/shift/${id}`, {
                        method: 'POST',
                        json: true,
                        body,
                });
                return result;
        }

        async _changeTime(processName, alpha, date) {
                let t = new Date(date).getTime();
                if (alpha === 0) {
                        throw new Error('You cannot freeze time.');
                }
                if (Date.now() - t > 1000) {
                        throw new Error('You cannot change your past, even black holes cannot do that.');
                }
                const {seconds, nanoseconds} = this._getSecondsAndNanoseconds(t);
                return this._sendChangeTimeRequest(processName, alpha, seconds, nanoseconds);
        }

        async mockDateForProcess(processName, date) {
                return this._changeTime(processName, 1, date);
        }

        async changeSpeedForProcess(processName, alpha) {
                return this._changeTime(processName, alpha, Date.now());
        }

        async hackTimeForProcess(processName, alpha, date) {
                return this._changeTime(processName, alpha, date);
        }

        async shiftTimeForProcess(processName, alpha, date) {
                let t = new Date(date).getTime();
                const {seconds, nanoseconds} = this._getSecondsAndNanoseconds(t);
                return this._sendShiftTimeRequest(processName, alpha, seconds, nanoseconds);        
        }

        async mockDate(date) {
                return Promise.all(this.processes.map(p => this.mockDateForProcess(p, date)));
        }

        async changeSpeed(alpha) {
                return Promise.all(this.processes.map(p => this.changeSpeedForProcess(p, alpha)));
        }

        async hackTime(alpha, date) {
                return Promise.all(this.processes.map(p => this.hackTime(p, alpha, date)));
        }

        async shiftTime(alpha = 1, date) {
                return Promise.all(this.processes.map(p => this.shiftTimeForProcess(p, alpha, date)));
        }
}

const sands = new Sands();
module.exports = sands