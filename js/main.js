import {sayHello, testCallback} from './func.js';

/*(async function() {
    var a = await sayHello();
    console.log(a);
})();

testCallback(function() {
    console.log('test callback output');
});*/

(async () => {
    for(let i = 0; i < 999; i++) {
        let content = await fs.readFileAsync();
        console.log(i + '. file content: ' + content);
    }
})();

