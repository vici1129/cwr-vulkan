// Scalar predicates also compiled by the CPU regression tests. Metres, not
// nonlinear hardware depth: allow small slopes while retaining CSM at edges.
bool rtSameDepth(float a, float b) {
    return a>0 && b>0 && abs(a-b)<=max(.02f,min(a,b)*.002f);
}
bool rtSameLight(float a, float b) {
    return abs(a-b)<=max(.0001f,abs(a)*.02f);
}
