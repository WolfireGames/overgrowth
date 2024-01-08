enum grabber_types { scaler, mover };

class DrikaUIGrabber{
	IMImage@ image;
	int direction_x;
	int direction_y;
	grabber_types grabber_type;
	string grabber_name;
	bool visible = false;
	int index = 0;
	int grabber_size = 35;
	vec2 size = vec2(grabber_size);
	float margin = 50.0;

	DrikaUIGrabber(string name, int _direction_x, int _direction_y, grabber_types _grabber_type){
		grabber_type = _grabber_type;

		if(grabber_type == scaler){
			IMImage grabber_image("Textures/UI/drika_ui_grabber.png");
			@image = grabber_image;
		}else{
			IMImage grabber_image("Textures/UI/drika_ui_empty.png");
			@image = grabber_image;
		}

		direction_x = _direction_x;
		direction_y = _direction_y;

	    IMMessage on_enter("grabber_activate");
		on_enter.addString(name);
	    IMMessage on_over("grabber_move_check");
		IMMessage on_exit("grabber_deactivate");

		grabber_name = imGUI.getUniqueName("grabber");

		image.setClip(false);
		image.addMouseOverBehavior(IMFixedMessageOnMouseOver( on_enter, on_over, on_exit ), "");
		image.setSize(size);
		grabber_container.addFloatingElement(image, grabber_name, vec2(0.0), 0);
	}

	void SetSize(vec2 _size){
		size = _size;
		image.setSize(vec2(abs(size.x), abs(size.y)) - vec2(margin));
	}

	void SetPosition(vec2 position){
		if(grabber_type == scaler){
			grabber_container.moveElement(grabber_name, position - (size / 2.0));
		}else{
			grabber_container.moveElement(grabber_name, vec2(size.x < 0.0?size.x+position.x:position.x, size.y < 0.0?size.y+position.y:position.y) + (vec2(margin / 2.0)));
		}
	}

	void SetZOrder(int parent_index){
		image.setZOrdering(parent_index + 1);
	}

	vec2 GetPosition(){
		return grabber_container.getElementPosition(grabber_name) + (size / 2.0);
	}

	void Delete(){
		grabber_container.removeElement(grabber_name);
	}

	void SetVisible(bool _visible){
		visible = _visible;
		image.setVisible(visible);
		image.setPauseBehaviors(!visible);
	}
}
