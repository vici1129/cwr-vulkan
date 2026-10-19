// Small scalar policy shared by the resolver and focused CPU tests.
bool giHistorySurface(float oldDepth,float expectedDepth,float normalDot) {
    return expectedDepth>0 && normalDot>=.95 &&
        abs(oldDepth-expectedDepth)<=max(.02,.002*expectedDepth);
}
float giHistoryLength(float previousLength,float maximum) { return min(previousLength+1,maximum); }
float giHistoryLength(float previousLength) { return giHistoryLength(previousLength,4.f); }
float giHistoryWeight(float length) { return 1-1/length; }
float giSampleRotation(float phase) { return phase*2.39996323; }
float giSampleShift(float phase) { return phase*.61803398875; }
