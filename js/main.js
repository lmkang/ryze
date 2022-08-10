import {sayHello, testCallback} from './func.js';

/*(async function() {
    var a = await sayHello();
    console.log(a);
})();

testCallback(function() {
    console.log('test callback output');
});*/

(async () => {
    let content = await fs.readFileAsync();
    console.log('file content: ', content);
})();

