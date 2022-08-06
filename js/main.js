import {sayHello, testCallback} from './func.js';

/*(async function() {
    var a = await sayHello();
    console.log(a);
})();

testCallback(function() {
    console.log('test callback output');
});*/

Promise.resolve().then(() => {
    console.log(1)
}).then(() => {
    console.log(3)
    return Promise.resolve(3.5)
}).then(() => {
    console.log(5)
}).then(() => {
    console.log(7)
})

Promise.resolve().then(() => {
    console.log(2)
}).then(() => {
    console.log(4)
}).then(() => {
    console.log(6)
}).then(() => {
    console.log(8)
})