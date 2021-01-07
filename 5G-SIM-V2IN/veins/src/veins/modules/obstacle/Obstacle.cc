//
// Copyright (C) 2010-2018 Christoph Sommer <sommer@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <algorithm>

#include "veins/modules/obstacle/Obstacle.h"

using namespace veins;

using veins::Obstacle;

Obstacle::Obstacle(std::string id, std::string type, double attenuationPerCut, double attenuationPerMeter)
    : visualRepresentation(nullptr)
    , id(id)
    , type(type)
    , attenuationPerCut(attenuationPerCut)
    , attenuationPerMeter(attenuationPerMeter)
{
}

void Obstacle::setShape(Coords shape)
{
    coords = shape;
    bboxP1 = Coord(1e7, 1e7);
    bboxP2 = Coord(-1e7, -1e7);
    for (Coords::const_iterator i = coords.begin(); i != coords.end(); ++i) {
        bboxP1.x = std::min(i->x, bboxP1.x);
        bboxP1.y = std::min(i->y, bboxP1.y);
        bboxP2.x = std::max(i->x, bboxP2.x);
        bboxP2.y = std::max(i->y, bboxP2.y);
    }
}

const Obstacle::Coords& Obstacle::getShape() const
{
    return coords;
}

const Coord Obstacle::getBboxP1() const
{
    return bboxP1;
}

const Coord Obstacle::getBboxP2() const
{
    return bboxP2;
}

bool Obstacle::containsPoint(Coord point) const
{
    bool isInside = false;
    const Obstacle::Coords& shape = getShape();
    Obstacle::Coords::const_iterator i = shape.begin();
    Obstacle::Coords::const_iterator j = (shape.rbegin() + 1).base();
    for (; i != shape.end(); j = i++) {
        bool inYRangeUp = (point.y >= i->y) && (point.y < j->y);
        bool inYRangeDown = (point.y >= j->y) && (point.y < i->y);
        bool inYRange = inYRangeUp || inYRangeDown;
        if (!inYRange) continue;
        bool intersects = point.x < (i->x + ((point.y - i->y) * (j->x - i->x) / (j->y - i->y)));
        if (!intersects) continue;
        isInside = !isInside;
    }
    return isInside;
}

namespace {

double segmentsIntersectAt(const Coord& p1From, const Coord& p1To, const Coord& p2From, const Coord& p2To)
{
    double p1x = p1To.x - p1From.x;
    double p1y = p1To.y - p1From.y;
    double p2x = p2To.x - p2From.x;
    double p2y = p2To.y - p2From.y;

    double p1p2x = p1From.x - p2From.x;
    double p1p2y = p1From.y - p2From.y;
    double D = (p1x * p2y - p1y * p2x);

    double p1Frac = (p2x * p1p2y - p2y * p1p2x) / D;
    if (p1Frac < 0 || p1Frac > 1) return -1;

    double p2Frac = (p1x * p1p2y - p1y * p1p2x) / D;
    if (p2Frac < 0 || p2Frac > 1) return -1;

    return p1Frac;
}

Coord segmentsIntersectAtCoord(const Coord &sender, const Coord &receiver,
        const Coord &c1, const Coord &c2) {

    //ascent mOne/mTwo of the functional equation
    double denominatorOne = receiver.x - sender.x;
    double mOne = (receiver.y - sender.y) / (denominatorOne);
    if (denominatorOne == 0)
        mOne = 0;

    double denominatorTwo = c2.x - c1.x;
    double mTwo = (c2.y - c1.y) / (denominatorTwo);
    if (denominatorTwo == 0)
        mTwo = 0;

    //n --> y-axis intercept
    double nOne = receiver.y - (mOne * receiver.x);
    double nTwo = c2.y - (mTwo * c2.x);

    //intersection calculation
    double x = (nTwo - nOne) / (mOne - mTwo);
    double y = (mOne * nTwo - mTwo * nOne) / (mOne - mTwo);

    if (x < 0 || y < 0)
        return Coord(-1, -1, -1);

    return Coord(x, y, 0);
}

} // namespace

std::vector<double> Obstacle::getIntersections(const Coord& senderPos, const Coord& receiverPos) const
{
    std::vector<double> intersectAt;
    const Obstacle::Coords& shape = getShape();
    Obstacle::Coords::const_iterator i = shape.begin();
    Obstacle::Coords::const_iterator j = (shape.rbegin() + 1).base();
    for (; i != shape.end(); j = i++) {
        const Coord& c1 = *i;
        const Coord& c2 = *j;

        double i = segmentsIntersectAt(senderPos, receiverPos, c1, c2);
        if (i != -1) {
            intersectAt.push_back(i);
        }
    }
    std::sort(intersectAt.begin(), intersectAt.end());
    return intersectAt;
}

bool Obstacle::checkIntersectionWithObstacle(const Coord &senderPos,
        const Coord &receiverPos, const double hBuilding, AnnotationManager * annotations) const {

    bool result = false;
    const Obstacle::Coords &shape = getShape();
    Obstacle::Coords::const_iterator i = shape.begin();
    Obstacle::Coords::const_iterator j = (shape.rbegin() + 1).base();
    for (; i != shape.end(); j = i++) {
        const Coord &c1 = *i;
        const Coord &c2 = *j;
        ASSERT(c1.z == c2.z); //for the case the poly-buildings have a z-coordinate

        double i = segmentsIntersectAt(senderPos, receiverPos, c1, c2);
        if (i != -1) {
            Coord coord = segmentsIntersectAtCoord(senderPos, receiverPos, c1,
                    c2);
            if (coord.x != -1) {
                //find out the height of the
                Coord locationVector(senderPos.x, senderPos.y, senderPos.z);
                Coord directionVector(receiverPos.x - senderPos.x,
                        receiverPos.y - senderPos.y,
                        receiverPos.z - senderPos.z);

                //lambda has to be equal for all three equations
                double lambdaX = (coord.x - locationVector.x)
                        / directionVector.x;
                double lambdaY = (coord.y - locationVector.y)
                        / directionVector.y;
                ASSERT(abs(lambdaY - lambdaX) <= 0.0000001);
                //calculate the height where the intersection occurs in 3D
                double theHeight = locationVector.z
                        + lambdaX * directionVector.z; // the height of the intersection

                //we have to check whether the height is above the building height, if no, NLOS case is detected and no further checks have to be considered, if yes, continue

                //either hBuilding or the z-coordinate from
                double compareWith = (c1.z > 0) ? c1.z : hBuilding;

                if (theHeight < compareWith) {
                    //hit the building
                    if (annotations) {
                        coord.z = theHeight;
                        annotations->drawBubble(coord, "hit");
                    }
                    return true;

                }
                //
            }
        }
    }
    return result;
}

std::string Obstacle::getType() const {
    return type;
}

std::string Obstacle::getId() const
{
    return id;
}

double Obstacle::getAttenuationPerCut() const
{
    return attenuationPerCut;
}

double Obstacle::getAttenuationPerMeter() const
{
    return attenuationPerMeter;
}
