class DrikaAnimationGroup{
	string name;
	array<string> animations;
	DrikaAnimationGroup(string _name){
		name = _name;
	}
	void AddAnimation(string _animation){
		animations.insertLast(_animation);
	}
	void SortAlphabetically(){
		animations.sortAsc();
	}
	int Size(){
		return animations.size();
	}
}
