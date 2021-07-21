
#pragma once

class noncopyable
{
protected:
noncopyable(){};
~noncopyable(){};
private:
noncopyable(noncopyable&);
const noncopyable& operator=(const noncopyable&);
};