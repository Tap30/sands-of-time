const SandsServer = require('./server');

async function runSandsServer(port = 9394) {
        let sandsServer = new SandsServer('localhost', port);
        sandsServer.init();
        console.log(`Listening on port: ${port}`);

        return sandsServer;
}

module.exports = {runSandsServer};