console.log('hello world!');

(async () => {
    try {
        let fd = await fs.open('/home/share/ryze/js/file.txt');
        console.log('fd: ', fd);
        if(fd >= 0) {
            for(let i = 0; i < 16; i++) {
                let content = await fs.read(fd);
                console.log(i + '. content: ', content);
            }
            let err = await fs.close(fd);
            console.log('errno: ', err);
        }
    } catch(e) {
        console.log("Error: ", JSON.stringify(e));
    }
})();
