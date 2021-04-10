#![crate_type = "lib"]
#![no_std]
#![allow(unused)]

trait Love<Target> {
    fn miss(&mut self, target: &'static Target);
}

struct You {}

struct Me<'time> {
    miss_you_level: u128,
    my_life: &'time bool,
}

impl<'now> Me<'now> {
    fn see_you(&mut self) -> bool {
        false
    }
}

impl<'my_life> Love<You> for Me<'my_life> {
    fn miss(&mut self, target: &'static You) {
        while ! self.see_you() {
            unsafe {
                self.miss_you_level <<= 1;
            }
        }
    }
}

