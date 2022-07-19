import sayHello from './func.js';

(async function() {
    var a = await sayHello();
    console.log(a);
})();

