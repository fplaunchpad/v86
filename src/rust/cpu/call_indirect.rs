#[no_mangle]
pub fn call_indirect1(f: fn(u16), x: u16) { f(x); }

#[no_mangle]
pub fn call_indirect_registers(
    f: fn(u32, i32, i32, i32, i32, i32, i32, i32, i32, i32),
    state: u32,
    eax: i32, ecx: i32, edx: i32, ebx: i32,
    esp: i32, ebp: i32, esi: i32, edi: i32,
    budget: i32,
) {
    f(state, eax, ecx, edx, ebx, esp, ebp, esi, edi, budget);
}
