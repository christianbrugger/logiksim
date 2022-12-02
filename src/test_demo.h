
class Tree {
public:
	class Leaf;

	explicit Tree(int leaf_count);
	Leaf leaf(int id);
	auto leafs();
private:
	int leaf_count_;
};


class Tree::Leaf {
public:
	explicit Leaf(int id);
	int id();
private:
	int id_;
};


Tree::Tree(int leaf_count) : leaf_count_(leaf_count) {
}

Tree::Leaf Tree::leaf(int id) {
	return Leaf{ id };
}

auto Tree::leafs() {
	return std::views::iota(0, leaf_count_) | std::views::transform(
		[this](int id) { return this->leaf(id); });
}

Tree::Leaf::Leaf(int id) : id_(id) {
}

int Tree::Leaf::id() {
	return id_;
}


void demo() {
	Tree tree{ 5 };
	std::ranges::for_each(tree.leafs(), [](Tree::Leaf leaf) {
		std::cout << leaf.id() << '\n';
		});
}

