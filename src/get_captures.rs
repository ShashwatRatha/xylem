// use std::collections::HashMap;
use tree_sitter::{Node, Point, Query, QueryCursor, Range, StreamingIterator, Tree};

pub fn get_name_map(src: &String, tree: &Tree, cursor: &mut QueryCursor, query: &Query) -> Option<()> {
    let mut matches = cursor.matches(&query, tree.root_node(), src.as_bytes());

    while let Some(m) = matches.next() {
        let mut args = "";
        let mut def: Node = Node::from(m.captures.iter().next()?.node);
        let mut body = "";
        let mut name = "";
        let mut func_def_range: Range = Range{
            start_byte: 0,
            end_byte: 0,
            start_point: Point{
                row:0,
                column: 0},
            end_point: Point{
                row: 0,
                column: 0}
        };

        for capture in m.captures {
            let capture_name = &query.capture_names()[capture.index as usize];
            if let Ok(text) = capture.node.utf8_text(src.as_bytes()) {
                match *capture_name {
                    "function.name" => name = text,
                    "function.def" => {
                        def = capture.node; 
                        func_def_range = capture.node.range();
                    },
                    "function.args" => args = text,
                    "function.body" => body = text,
                    _ => ()
                }
            }
        }

        if !name.is_empty() && !args.is_empty() && !body.is_empty() {
            println!("{name}{args} {body} RNG: [{} - {}]",
                func_def_range.start_point, func_def_range.end_point);
        }
    }

    
    Some(())
}
