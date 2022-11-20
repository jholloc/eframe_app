__ATPOSTRUN__.push(() => {
    let a = FS.readFile("output.bmp");
    fs.writeFile("output.bmp", a, err => {
        if (err) return console.log(err);
    });
});