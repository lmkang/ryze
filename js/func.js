import {sort} from './utils.js';

export default async function() {
    return new Promise((resolve, reject) => {
        var content = fs.read(path.join(import.meta.dirname, 'file.txt'));
        resolve(content);
    });
}

console.log(import.meta.url, import.meta.dirname);

var arr = [0, 5, 2, 9, 8, 3, 1, 4, 6, 7];
sort(arr);
console.log(arr);
