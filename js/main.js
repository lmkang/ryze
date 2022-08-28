console.log('hello world!');

(async () => {
    let fd = await fs.open('./js/file.txt');
    console.log('fd: ', fd);
    if(fd >= 0) {
        let err = await fs.close(fd);
        console.log('errno: ', err);
    }
})();
