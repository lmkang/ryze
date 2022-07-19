export default async function() {
    var content = fs.read('./file.txt');
    console.log(content);
    return 'aaa';
}
