import {sort} from './utils.js';

export default async function() {
    var content = fs.read('./file.txt');
    console.log(content);
    return 'aaa';
}

var arr = [0, 5, 2, 9, 8, 3, 1, 4, 6, 7];
sort(arr);
console.log(arr);
