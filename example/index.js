const sands = require('../lib/index');
const {execSync} = require('child_process')
const path = require('path');

async function main() {
        try {
                await sands.runServer();
                const node = execSync('which node').toString().replace(/\r?\n|\r/g, '');
                const pathToTest = path.join(__dirname, './log_date.js').toString();
                console.log('Spawning "log_date"');
                await sands.spawnProcess('log_date', node || 'node',  [pathToTest], {}, '/tmp');
                console.log('Please check the following file: /tmp/log_date.sands.log');
                setTimeout(async () => {
                        console.log('Changing the speed of time');
                        await sands.shiftTime(2, 0);
                        console.log('Done');
                }, 20000);
        } catch (e) {
                console.error(e);
        }
}

main();