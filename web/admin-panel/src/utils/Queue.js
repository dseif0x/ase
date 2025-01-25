import {Subject, zip} from "rxjs";

const asyncSub = new Subject();
const nextAsyncSub = new Subject();
zip(asyncSub, nextAsyncSub).subscribe({
    next: ([fn]) => {
        fn().finally(() => nextAsyncSub.next())
    }
});
nextAsyncSub.next();
export function queue(fn) {
    asyncSub.next(fn);
}