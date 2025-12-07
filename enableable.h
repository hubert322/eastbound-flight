#ifndef ENABLEABLE_H
#define ENABLEABLE_H

class Enableable {
private:
	bool isEnabled_ = false;

public:
	virtual ~Enableable() = default;

	void setEnabled(bool enabled) {
		isEnabled_ = enabled;
	}

	bool isEnabled() {
		return  isEnabled_;
	}
};

#endif