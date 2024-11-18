import csv
import os
from json import loads

class BusinessObject:
    def __init__(self, data):
        self.business_id = format_str(data.get("business_id"))
        self.name = format_str(data.get("name"))
        self.address = format_str(data.get("address"))
        self.city = format_str(data.get("city"))
        self.state = format_str(data.get("state"))
        self.postal_code = format_str(data.get("postal_code"))
        self.latitude = data.get("latitude")
        self.longitude = data.get("longitude")
        self.stars = data.get("stars")
        self.review_count = data.get("review_count")
        self.is_open = data.get("is_open")
    
    def get_object(self):
        return [
            self.business_id, 
            self.name, 
            self.address, 
            self.city, 
            self.state, 
            self.postal_code, 
            self.latitude, 
            self.longitude, 
            self.stars, 
            self.review_count, 
            self.is_open
        ]


class ReviewObject:
    def __init__(self, data):
        self.review_id = format_str(data.get("review_id"))
        self.user_id = format_str(data.get("user_id"))
        self.business_id = format_str(data.get("business_id"))
        self.stars = data.get("stars")
        self.date = format_date(data.get("date"))
        self.text = format_str(data.get("text"))
        self.useful = data.get("useful")
        self.funny = data.get("funny")
        self.cool = data.get("cool")
    
    def get_object(self):
        return [
            self.review_id,
            self.user_id,
            self.business_id,
            self.stars,
            self.date,
            self.text,
            self.useful,
            self.funny,
            self.cool
        ]


class TipObject:
    def __init__(self, data):
        self.text = format_str(data.get("text"))
        self.date = format_date(data.get("date"))
        self.compliment_count = data.get("compliment_count")
        self.business_id = format_str(data.get("business_id"))
        self.user_id = format_str(data.get("user_id"))
    
    def get_object(self):
        return [
            self.text,
            self.date,
            self.compliment_count,
            self.business_id,
            self.user_id
        ]


class UserObject:
    def __init__(self, data):
        self.user_id = format_str(data.get("user_id"))
        self.name = format_str(data.get("name"))
        self.review_count = data.get("review_count")
        self.yelping_since = format_date(data.get("yelping_since"))
        self.friends = format_str(data.get("friends").split(",")[0]) if data.get("friends") != '' else None
        self.useful = data.get("useful")
        self.funny = data.get("funny")
        self.cool = data.get("cool")
        self.fans = data.get("fans")
        self.elite = format_str(data.get("elite").split(",")[0]) if data.get("elite") != '' else None
        self.average_stars = data.get("average_stars")
        self.total_compliments = (
            data.get("compliment_hot")
            + data.get("compliment_more")
            + data.get("compliment_profile")
            + data.get("compliment_cute")
            + data.get("compliment_list")
            + data.get("compliment_note")
            + data.get("compliment_plain")
            + data.get("compliment_cool")
            + data.get("compliment_funny")
            + data.get("compliment_writer")
            + data.get("compliment_photos")
        )
    
    def get_object(self):
        return [
            self.user_id,
            self.name,
            self.review_count,
            self.yelping_since,
            self.friends,
            self.useful,
            self.funny,
            self.cool,
            self.fans,
            self.elite,
            self.average_stars,
            self.total_compliments
        ]
        

def format_date(input):
    arr = input.split(" ")
    return arr[0] + " " + arr[1]


def format_str(input):
    return "\"" + input.replace('"', '""').replace("\n", " ").replace("\t", " ").replace("\r", " ").strip() + "\""


def parseJson(json_file):
    file_name = os.path.splitext(os.path.basename(json_file))[0]
    output_file = open("data/" + file_name + ".tsv", "w", newline="")
    
    data = []
    if file_name == "yelp_academic_dataset_business":
        data.append(["business_id", "name", "address", "city", "state", "postal_code", "latitude", "longitude", "stars", "review_count", "is_open"])
    elif file_name == "yelp_academic_dataset_review":
        data.append(["review_id", "user_id", "business_id", "stars", "date", "text", "useful", "funny", "cool"])
    elif file_name == "yelp_academic_dataset_tip":
        data.append(["text", "date", "compliment_count", "business_id", "user_id"])
    else:
        data.append(["user_id", "name", "review_count", "yelping_since", "friends", "useful", "funny", "cool", "fans", "elite", "average_stars", "total_compliments"])
    
    with open(json_file, 'r', encoding="utf-8") as file:
        # content = file.read()
        # content = content.replace("\n", "\\n").replace("\t", "\\t")
        
        for line in file:
            try:
                json_line = loads(line)
            except Exception as e:
                print(line)
                print(e)
                continue
            
            # print(json_line)
            
            obj = None
            if file_name == "yelp_academic_dataset_business":
                obj = BusinessObject(json_line)
            elif file_name == "yelp_academic_dataset_review":
                obj = ReviewObject(json_line)
            elif file_name == "yelp_academic_dataset_tip":
                obj = TipObject(json_line)
            else:
                obj = UserObject(json_line)
            
            data.append(obj.get_object())
        
        writer = csv.writer(output_file, delimiter="\t")
        writer.writerows(data)

print("Creating business.tsv...")
parseJson("data/yelp_dataset/yelp_academic_dataset_business.json")
print("Done!")

print("Creating tip.tsv...")
parseJson("data/yelp_dataset/yelp_academic_dataset_tip.json")
print("Done!")

print("Creating user.tsv...")
parseJson("data/yelp_dataset/yelp_academic_dataset_user.json")
print("Done!")

print("Creating review.tsv...")
parseJson("data/yelp_dataset/yelp_academic_dataset_review.json")
print("Done!")