function logDate(ms) {
        setTimeout(() => {
                console.log(new Date());
                logDate(ms);
        }, ms);
}

logDate(5000);