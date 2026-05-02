#include "BuildOrderPlayback.h"

int FindCurrentStepIndex(const BuildOrder& order, double dElapsedSeconds)
{
    const int n = static_cast<int>(order.vSteps.size());
    if (n < 1)
        return -1;
    int iBest = -1;
    for (int i = 0; i < n; ++i)
    {
        const BuildStep& st = order.vSteps[static_cast<size_t>(i)];
        if (!st.bTimeSpecified)
            continue;
        if (static_cast<double>(st.fTimeSec) <= dElapsedSeconds)
            iBest = i;
    }
    if (iBest < 0)
        return 0;
    return iBest;
}
