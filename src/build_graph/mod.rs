pub mod defs;

use tree_sitter::{Query, QueryCursor, Range, StreamingIterator, Tree};
use self::defs::FuncDef;

pub fn enumeration_pass(src: &str, tree: &Tree, cursor: &mut QueryCursor, query: &Query,
    file: &str) -> Vec<FuncDef>{
    let mut matches = cursor.matches(query, tree.root_node(), src.as_bytes());
    let mut funcs: Vec<FuncDef> = Vec::new();
    
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
            funcs.push(FuncDef::new(name, file, rng.start_byte, rng.end_byte, is_static));
        }
    }

    funcs
}
