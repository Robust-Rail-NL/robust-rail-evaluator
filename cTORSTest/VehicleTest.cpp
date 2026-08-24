#include "doctest/doctest.h"
#include "Train.h"
#include "ShuntingUnit.h"

namespace cTORSTest {
    TEST_CASE("Train Test") {
        TrainUnitType elecTrainType("ElecTrainType", 1, 100, 100, 100, 100, 100, 50, 100, "ETT", false, false, true);
        TrainUnitType nonElecTrainType("NonElecTrainType", 1, 100, 100, 100, 100, 100, 50, 100, "NETT", false, false, false);
        CHECK(nonElecTrainType != elecTrainType);
        Train elecTrain(0, &elecTrainType);
        Train nonElecTrain(1, &nonElecTrainType);
        CHECK(elecTrain != nonElecTrain);
        CHECK(elecTrain == Train(elecTrain));
        CHECK(Train(-1, &nonElecTrainType) != Train(-1, &nonElecTrainType));

        TrainUnitType::types[elecTrainType.displayName] = &elecTrainType;
        TrainUnitType::types[nonElecTrainType.displayName] = &nonElecTrainType;

        //TODO test shunting units, length, needsElectric?
    }

    TEST_CASE("TrainUnitType carriage disambiguation") {
        // Two variants of the same family, differing only in carriages - the
        // scenario that motivated keying TrainUnitType identity on
        // (displayName, carriages) rather than displayName alone.
        TrainUnitType slt4("SLT", 4, 100, 100, 100, 100, 100, 50, 100, "SLT", false, false, true);
        TrainUnitType slt6("SLT", 6, 150, 100, 100, 100, 100, 50, 100, "SLT", false, false, true);

        CHECK(slt4 != slt6);
        CHECK(slt4 == TrainUnitType("SLT", 4, 999, 1, 1, 1, 1, 1, 1, "SLT", true, true, false));

        // Unspecified-ID trains (id == -1) are matched by type alone - this is the
        // "any train unit of this type will do" case for outgoing TrainRequests.
        Train wantSlt4(-1, &slt4);
        Train wantSlt6(-1, &slt6);
        ShuntingUnit requestSlt4(1, {wantSlt4});
        ShuntingUnit requestSlt6(2, {wantSlt6});

        // A shunting unit actually made of an SLT-4 must NOT satisfy a request for
        // an SLT-6, even though both share the "SLT" family name.
        CHECK(!requestSlt4.MatchesShuntingUnit(&requestSlt6));
        CHECK(!requestSlt6.MatchesShuntingUnit(&requestSlt4));
        CHECK(requestSlt4.MatchesShuntingUnit(&requestSlt4));

        CHECK(!requestSlt4.MatchesTrainIDs({-1}, {&slt6}));
        CHECK(requestSlt4.MatchesTrainIDs({-1}, {&slt4}));
    }

}