#pragma once
#ifndef FACE_ENROLL_H
#define FACE_ENROLL_H

#include "command_base.h"
using namespace std;

/* Client request: action = "enroll" | "delete" | "clear" */
class FaceEnroll : public CommandBase
{
public:
    FaceEnroll() : CommandBase("face_enroll") {}

    bool Deserizlie(std::string msg)
    {
        shared_ptr<FaceEnroll> dat = CommandCreateHelper::DeserializeCommandBody<FaceEnroll>(msg);
        if (dat == NULL) return false;
        command_name = dat->command_name;
        action = dat->action;
        name = dat->name;
        return true;
    }

    template<class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(action), CEREAL_NVP(name));
    }

    template<class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(action), CEREAL_NVP(name));
    }

public:
    string action;
    string name;
};
#endif
