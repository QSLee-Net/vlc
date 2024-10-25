/// SPDX-License-Identifier: LGPL-2.1-or-later
/// Copyright (C) 2024-2025 Alexandre Janniaux <ajanni@videolabs.io>
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published by
/// the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public License
/// along with this program; if not, write to the Free Software Foundation,
/// Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
use std::{
    cell::UnsafeCell,
    ffi::{c_char, c_void, CStr},
    mem::MaybeUninit,
    ptr::NonNull,
};

use crate::{object::Object, plugin::ModuleProtocol};

pub mod sys;
pub use sys::{Trace, TraceField};

pub struct Tick(pub i64);

///
/// Trait representing the module capability for implementing tracers.
///
/// A type implementing the [TracerCapability] trait can be exposed to
/// a [vlcrs_macros::module!] manifest using the
/// [TracerModuleLoader] type loader, which will expose it as a "tracer"
/// capability for VLC core.
///
/// Note that this trait *requires* [Sync] since the tracer is used
/// from the C code in multiple threads at the same time.
///
/// [Sync] is automatically implemented by types using only [Sync]
/// types, and is unsafe to implement. It means that the following
/// implementation would not be possible without unsafe:
///
/// ```compile_fail
/// use std::cell::Cell;
/// use vlcrs_core::object::Object;
/// use vlcrs_core::tracer::{Tick, TracerCapability, Trace};
/// struct Module { last_trace_tick: Cell<Tick>, }
/// impl TracerCapability for Module {
///     fn open(obj: &mut Object) -> Option<impl TracerCapability> {
///         Some(Self{ last_trace_tick: Cell::from(Tick(0)) })
///     }
///
///     fn trace(&self, tick: Tick, entries: &Trace) {
///         let mut state = self.last_trace_tick.get_mut();
///         *state = tick;
///     }
/// }
/// ````
///
/// As it will fail with the following error:
///
/// ```no_build
///     test: vlcrs-core/src/tracer/mod.rs - tracer::TracerCapability (line 31)
///     error[E0277]: `Cell<Tick>` cannot be shared between threads safely
///       --> vlcrs-core/src/tracer/mod.rs:36:27
///        |
///     8  | impl TracerCapability for Module {
///        |                           ^^^^^^ `Cell<Tick>` cannot be shared between threads safely
///        |
///        = help: within `Module`, the trait `Sync` is not implemented for `Cell<Tick>`, which is required by `Module: Sync`
///        = note: if you want to do aliasing and mutation between multiple threads, use `std::sync::RwLock`
///     note: required because it appears within the type `Module`
/// ````
///
/// Instead, proper concurrency handling must be implemented inside the
/// object so that it becomes Sync and can be used in the multiples threads.
///
pub trait TracerCapability: Sync {
    fn open(obj: &mut Object) -> Option<impl TracerCapability>
    where
        Self: Sized;

    fn trace(&self, tick: Tick, entries: &Trace);
}

#[allow(non_camel_case_types)]
pub type TracerCapabilityActivate =
    unsafe extern "C" fn(
        obj: &mut Object,
        opaque: &mut MaybeUninit<*mut c_void>,
    ) -> Option<&'static sys::vlc_tracer_operations>;

extern "C" fn tracer_trace(opaque: *const c_void, tick: sys::vlc_tick, entries: sys::Trace) {
    {
        let tracer: &dyn TracerCapability =
            unsafe { &**(opaque as *const Box<dyn TracerCapability>) };
        tracer.trace(Tick(tick), &entries);
    }
}

extern "C" fn tracer_destroy(opaque: *mut c_void) {
    let tracer: *mut Box<dyn TracerCapability> = opaque as *mut _;
    let _ = unsafe { Box::from_raw(tracer) };
}

const TRACER_OPERATIONS: sys::vlc_tracer_operations = sys::vlc_tracer_operations {
    trace: tracer_trace,
    destroy: tracer_destroy,
};

extern "C" fn activate_tracer<T: TracerCapability>(
    obj: &mut Object,
    opaque: &mut MaybeUninit<*mut c_void>,
) -> Option<&'static sys::vlc_tracer_operations> {
    if let Some(instance) = T::open(obj) {
        let wrapper: Box<dyn TracerCapability> = Box::try_new(instance).ok()?;
        let sys = Box::into_raw(Box::try_new(wrapper).ok()?);
        opaque.write(sys as *mut _);
        return Some(&TRACER_OPERATIONS);
    }
    None
}

///
/// Loader type for the "tracer" capability, using the
/// [TracerCapability] trait.
///
/// This is an implementation of the [ModuleProtocol] type for the
/// [TracerCapability] trait, exposing modules from the declaration
/// ([`vlcrs_macros::module!`]) of the manifest as a "tracer"
/// capability for VLC core.
///
/// ```
/// use vlcrs_core::tracer::{Tick, TracerCapability, TracerModuleLoader, Trace};
/// use vlcrs_core::object::Object;
/// use vlcrs_macros::module;
/// struct CustomTracer {}
///
/// // Empty implementation
/// impl TracerCapability for CustomTracer {
///     fn open(obj: &mut Object) -> Option<impl TracerCapability> {
///         Some(Self{})
///     }
///     fn trace(&self, _tick: Tick, _entries: &Trace) {}
/// }
///
/// module!{
///    type: CustomTracer (TracerModuleLoader),
///    capability: "tracer" @ 0,
///    category: ADVANCED_MISC,
///    description: "Custom tracer implementation",
///    shortname: "Custom tracer",
///    shortcuts: ["customtracer"],
///}
/// ````
pub struct TracerModuleLoader;

impl<T> ModuleProtocol<T> for TracerModuleLoader
where
    T: TracerCapability,
{
    type Activate = TracerCapabilityActivate;
    fn activate_function() -> Self::Activate {
        activate_tracer::<T>
    }
}

///
/// Wrapper around a vlc_tracer implementation from VLC core.
///
/// Exposes the public API for using tracers from VLC core.
///
pub struct Tracer {
    tracer: UnsafeCell<NonNull<sys::vlc_tracer>>,
}

impl Tracer {
    ///
    /// Request a new instance of a tracer from LibVLC.
    ///
    pub fn create(obj: &Object, name: &CStr) -> Option<Tracer> {
        let name = name.as_ptr() as *const c_char;

        // SAFETY: sys::vlc_tracer_Create() is safe as long as name is
        //         null-terminated (given by CStr) and Object is valid,
        //         which is given by requiring a reference.
        let tracer: NonNull<sys::vlc_tracer> = unsafe { sys::vlc_tracer_Create(obj, name)? };

        Some(Self {
            tracer: UnsafeCell::new(tracer),
        })
    }

    ///
    /// Register the new point at time [tick] with the metadata [entries].
    ///
    pub fn trace(&self, tick: Tick, entries: Trace) {
        unsafe {
            // SAFETY: TODO
            let tracer = *self.tracer.get();

            // SAFETY: the pointer `tracer` is guaranteed to be non-null and
            //         nobody else has reference to it.
            sys::vlc_tracer_TraceWithTs(tracer, tick.0, entries);
        }
    }
}

#[macro_export]
macro_rules! trace {
    ($tracer: ident, $ts: expr, $($key:ident = $value:expr)*) => {};
}
