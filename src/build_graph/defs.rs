use std::collections::HashSet;
use std::fmt::Display;

pub struct FuncDef {
    name: String,
    file: String,
    start: usize,
    end: usize,
    is_static: bool
}

impl FuncDef {
    pub fn new(name: &str, file: &str, start: usize, end: usize,
        is_static: bool) -> FuncDef {
        FuncDef { 
            name: String::from(name),
            file: String::from(file),
            start: start,
            end: end,
            is_static: is_static
        }
    }
}

pub struct GraphNode {
    id: String,
    callers: HashSet<String>,
    callees: HashSet<String>
}

impl GraphNode {
    pub fn new(id: &str) -> GraphNode {
        GraphNode { 
            id: String::from(id),
            callers: HashSet::new(),
            callees: HashSet::new() 
        }
    }

    pub fn insert_caller(&mut self, caller_name: &str) -> () {
        self.callers.insert(String::from(caller_name));
    }

    pub fn insert_callee(&mut self, callee_name: &str) -> () {
        self.callees.insert(String::from(callee_name));
    }
}
