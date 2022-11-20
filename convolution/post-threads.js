__ATPOSTRUN__.push(() => {
    let done = false;
    let try_write = () => {
        let a = FS.readFile("output.bmp");
        fs.writeFile("output.bmp", a, err => {
            if (err) return console.log(err);
        });
        done = true;
    };
    let interval = setInterval(() => {
        try {
            if (!done) {
                try_write();
                console.log("writen output.bmp");
                clearInterval(interval);
                // PThread.terminateAllThreads();
            }
        } catch (e) {
        }
    }, 100);
});