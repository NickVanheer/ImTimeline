#pragma once

    class IDGenerator
    {
    public:
        int GetUniqueID()
        {
            mID++;
            return mID;
        }
        private:
        s32 mID = 0;
    };

    // static int GetHash(const std::string& a_str)
    // {
    //     return 1;
    //     // std::hash<std::string> hashHelper;

    //     // return hashHelper(a_str);
    // }