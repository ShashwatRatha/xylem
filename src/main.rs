use tree_sitter::Parser;

fn main() {
    let mut c_parser = Parser::new();

    c_parser.set_language(&tree_sitter_c::LANGUAGE.into())
        .expect("Failed loading C lang");

    let src = std::fs::read_to_string("src/main.c")
        .expect("Error opening C file");

    let tree = c_parser.parse(&src, None).unwrap();

    println!("{}", tree.root_node().to_sexp());
}
