use std::str::Utf8Error;

// use std::collections::HashMap;
use tree_sitter::{Node, Query, QueryCursor, Range, StreamingIterator, Tree};

// use self::defs::FuncDef;

mod defs;

pub fn print_function_map(src: &str, tree: &Tree, cursor: &mut QueryCursor, query: &Query,
    file: &str) {
    let mut matches = cursor.matches(query, tree.root_node(), src.as_bytes());
    
    while let Some(m) = matches.next() {
        let mut name: Option<&str> = None;
        let mut range: Option<Range> = None;
        let mut is_static: bool = false;

        for capture in m.captures {
            let capture_name = query.capture_names()[capture.index as usize];
            let text = capture.node.utf8_text(src.as_bytes()).ok();

            match capture_name {
                "fn.name" => name = text,
                "fn.is_static" | "fn.is_non_static" => {
                    is_static = capture_name == "fn.is_static";
                    range = Some(capture.node.range());
                },
                _ => {}
            }

        }

        if let (Some(name), Some(rng)) = (name, range) {
            // let func = FuncDef::new(name, file, rng.start_byte, rng.end_byte,
                // storage.contains("static"));
            println!("{name} {} - {} {is_static}", rng.start_point, rng.end_point);
        }
    }
}
