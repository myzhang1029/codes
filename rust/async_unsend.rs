//! Test stuff when a `Fn` argument is `!Send`.
use core::cell::UnsafeCell;
use core::future::Future;
use core::pin::Pin;
use core::ptr::null;
use core::task::{Context, RawWaker, RawWakerVTable, Waker};

unsafe fn clone_vtable(data: *const ()) -> RawWaker {
    println!("clone");
    RawWaker::new(data, &VTABLE)
}

unsafe fn wake_vtable(_data: *const ()) {
    println!("wake");
}

unsafe fn wake_by_ref_vtable(_data: *const ()) {
    println!("wake_by_ref");
}

unsafe fn drop_vtable(_data: *const ()) {
    println!("drop");
}

const VTABLE: RawWakerVTable =
    RawWakerVTable::new(clone_vtable, wake_vtable, wake_by_ref_vtable, drop_vtable);

#[allow(clippy::unused_async)]
async fn test_fn<F>(f: F)
where
    F: Fn(),
{
    f();
}

async fn wrap_fn<F>(f: F)
where
    F: Fn(),
{
    test_fn(f).await;
}

fn main() {
    let make_it_unsync = UnsafeCell::new(10);
    let make_it_unsend = UnsafeCell::raw_get(&make_it_unsync);
    let f = move || {
        println!("Hello, world!");
        let inner = unsafe { &*make_it_unsync.get() };
        println!("inner: {inner}");
        let inner = unsafe { &*make_it_unsend };
        println!("inner: {inner}");
    };
    let raw_waker = RawWaker::new(null(), &VTABLE);
    let waker = unsafe { Waker::from_raw(raw_waker) };
    let mut f = wrap_fn(f);
    let mut cx = Context::from_waker(&waker);
    let f = unsafe { Pin::new_unchecked(&mut f) };
    let poll_result = f.poll(&mut cx);
    println!("{poll_result:?}");
}
