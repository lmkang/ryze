import {sort} from './utils.js';

export async function sayHello() {
    return new Promise((resolve, reject) => {
        var content = fs.read(path.join(import.meta.dirname, 'file.txt'));
        resolve(content);
    });
}

export function testCallback(callback) {
    console.log('test callback');
    callback();
}

console.log(import.meta.url, import.meta.dirname);

var arr = [0, 5, 2, 9, 8, 3, 1, 4, 6, 7];
sort(arr);
console.log(arr);
