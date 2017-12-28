/* Observer Pattern v0.1
 * 2017-12-27
 *
 * Subject	(base)				Observer (base)
 *  notify() --- update -------> Obs1 (derived)
 *		     ---- update ----------------------> Obs2 (dervied)		 
 *  <------------- getDouble --- update()
 *	<--- getDouble ----------------------------- update()
 */

#pragma once

using namespace std;

void updateObserver(class Observer* pObserver);

class Subject {
protected:
	list<Observer*> mObservers;
public:
	Subject() {}
	void attach(Observer* pObserver) { mObservers.push_back(pObserver); }
	void detach(Observer* pObserver) { mObservers.remove(pObserver); }
	void notifyObservers() {
		for_each(mObservers.begin(), mObservers.end(), updateObserver);
	}
	virtual double getDouble(string name) = 0 {};
	virtual string getParam(string name) = 0 {};
	virtual bool setParam(string name, string value) = 0 {};
};

class Observer {
protected:
	Subject* mSubject;
public:
	Observer(Subject* pSubject) : mSubject(pSubject) {}
	virtual void update() = 0 {}
};

